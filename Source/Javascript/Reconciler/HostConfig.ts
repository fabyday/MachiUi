import Reconciler from "react-reconciler";
import type { HostConfig } from "react-reconciler";
import type { ReactNode } from "react";
import { DefaultEventPriority } from "react-reconciler/constants";
import {
  attachChild,
  clearChildren as clearDomChildren,
  createHostInstance,
  createHostTextInstance,
  detachChild,
  detachDeletedNode,
  getDocumentBody,
  insertChildBefore as insertDomChildBefore,
  registerReactEventHandler,
  setDocumentRootNativePtr,
  syncHostProps,
} from "./DomShim";
import { NativeView, actions, MachiHost } from "./NativeHost";
import { resolveClassName } from "./StyleSheet";

type HostContext = null;
type PropUpdate = {
  kind: "prop" | "event";
  key: string;
  value: unknown;
};
type UpdatePayload = PropUpdate[];
type TimeoutHandle = number;
type NoTimeout = -1;

const INTERNAL_PROPS = new Set(["children", "key", "ref"]);
const pendingTimeouts = new Map<number, () => unknown>();
let timeoutHandle = 0;

function isEventProp(key: string) {
  return /^on[A-Z]/.test(key);
}

function isTextNode(node: HostNode): node is TextInstance {
  return node.type === "#text";
}

function getNativePtr(node: HostNode): NativePtr {
  return node.nativePtr;
}

function collectRenderableProps(props: Props) {
  const renderableProps: Record<string, unknown> = {
    ...resolveClassName(props.className),
  };

  for (const key of Object.keys(props)) {
    if (INTERNAL_PROPS.has(key) || key === "className" || isEventProp(key)) {
      continue;
    }

    const value = props[key];
    if (key === "style" && value != null && typeof value === "object") {
      for (const styleKey of Object.keys(value as Record<string, unknown>)) {
        renderableProps[styleKey] = (value as Record<string, unknown>)[styleKey];
      }
      continue;
    }

    if (typeof value !== "function") {
      renderableProps[key] = value;
    }
  }

  return renderableProps;
}

function forEachRenderableProp(
  props: Props,
  callback: (key: string, value: unknown) => void,
) {
  const renderableProps = collectRenderableProps(props);
  for (const key of Object.keys(renderableProps)) {
    callback(key, renderableProps[key]);
  }
}

function applyProps(nativePtr: NativePtr, props: Props) {
  forEachRenderableProp(props, (key, value) => {
    MachiNative.updateProps(nativePtr, key, value);
  });
}

function collectEventProps(props: Props) {
  const eventProps: Record<string, unknown> = {};
  for (const key of Object.keys(props)) {
    const value = props[key];
    if (isEventProp(key) && typeof value === "function") {
      eventProps[key] = value;
    }
  }
  return eventProps;
}

function applyEventHandlers(nativePtr: NativePtr, props: Props) {
  const eventProps = collectEventProps(props);
  for (const key of Object.keys(eventProps)) {
    registerReactEventHandler(nativePtr, key, eventProps[key]);
  }
}

function clearEventHandlers(nativePtr: NativePtr, props: Props) {
  const eventProps = collectEventProps(props);
  for (const key of Object.keys(eventProps)) {
    registerReactEventHandler(nativePtr, key, null);
  }
}

function diffProps(oldProps: Props, newProps: Props): UpdatePayload | null {
  const updates: UpdatePayload = [];
  const oldRenderableProps = collectRenderableProps(oldProps);
  const newRenderableProps = collectRenderableProps(newProps);
  const oldKeys = new Set(Object.keys(oldRenderableProps));

  for (const key of Object.keys(newRenderableProps)) {
    oldKeys.delete(key);
    const value = newRenderableProps[key];

    if (oldRenderableProps[key] !== value) {
      updates.push({ kind: "prop", key, value });
    }
  }

  for (const removedKey of oldKeys) {
    updates.push({ kind: "prop", key: removedKey, value: null });
  }

  const oldEventProps = collectEventProps(oldProps);
  const newEventProps = collectEventProps(newProps);
  const oldEventKeys = new Set(Object.keys(oldEventProps));

  for (const key of Object.keys(newEventProps)) {
    oldEventKeys.delete(key);
    const value = newEventProps[key];
    if (oldEventProps[key] !== value) {
      updates.push({ kind: "event", key, value });
    }
  }

  for (const removedKey of oldEventKeys) {
    updates.push({ kind: "event", key: removedKey, value: null });
  }

  return updates.length > 0 ? updates : null;
}

function scheduleMicrotaskCompat(fn: () => unknown) {
  if (typeof queueMicrotask === "function") {
    queueMicrotask(fn);
    return;
  }

  Promise.resolve().then(fn);
}

function normalizeContainer(container?: Container | NativePtr | string): Container {
  if (container != null && typeof container === "object" && "nativePtr" in container) {
    return container;
  }

  if (typeof container === "number" || typeof container === "bigint") {
    return {
      rootTag: "ExternalRoot",
      nativePtr: container,
    };
  }

  const sceneName = typeof container === "string" ? container : "DefaultScene";
  return {
    rootTag: sceneName,
    nativePtr: MachiNative.createRoot(sceneName),
  };
}

function getContainerDomTarget(container: Container | Instance) {
  return "childNodes" in (container as Record<string, unknown>) ? container : getDocumentBody();
}

const hostConfig: HostConfig<
  string,
  Props,
  Container,
  Instance,
  TextInstance,
  never,
  never,
  HostNode,
  HostContext,
  UpdatePayload,
  never,
  TimeoutHandle,
  NoTimeout
> = {
  supportsMutation: true,
  supportsPersistence: false,
  supportsHydration: false,
  isPrimaryRenderer: true,

  createInstance(type, props) {
    const nativePtr = MachiNative.createElement(type);
    applyProps(nativePtr, props);

    const instance = createHostInstance(type, nativePtr, props);
    syncHostProps(instance, props, collectRenderableProps(props));
    applyEventHandlers(nativePtr, props);
    return instance;
  },

  createTextInstance(text) {
    return createHostTextInstance(MachiNative.createTextNode(text), text);
  },

  appendInitialChild(parent, child) {
    attachChild(parent, child);
    MachiNative.appendChild(parent.nativePtr, getNativePtr(child));
  },

  finalizeInitialChildren() {
    return false;
  },

  shouldSetTextContent() {
    return false;
  },

  getRootHostContext() {
    return null;
  },

  getChildHostContext() {
    return null;
  },

  getPublicInstance(instance) {
    return instance;
  },

  prepareForCommit() {
    return null;
  },

  resetAfterCommit() {},

  preparePortalMount() {},

  prepareUpdate(_instance, _type, oldProps, newProps) {
    return diffProps(oldProps, newProps);
  },

  appendChild(parent, child) {
    attachChild(parent, child);
    MachiNative.appendChild(parent.nativePtr, getNativePtr(child));
  },

  appendChildToContainer(container, child) {
    const target = getContainerDomTarget(container as Container | Instance);
    attachChild(target, child);
    MachiNative.appendChild(container.nativePtr, getNativePtr(child));
  },

  insertBefore(parent, child, beforeChild) {
    insertDomChildBefore(parent, child, beforeChild);
    MachiNative.insertBefore(parent.nativePtr, getNativePtr(child), getNativePtr(beforeChild));
  },

  insertInContainerBefore(container, child, beforeChild) {
    insertDomChildBefore(getContainerDomTarget(container as Container | Instance), child, beforeChild);
    MachiNative.insertBefore(container.nativePtr, getNativePtr(child), getNativePtr(beforeChild));
  },

  removeChild(parent, child) {
    detachChild(parent, child);
    MachiNative.removeChild(parent.nativePtr, getNativePtr(child));
  },

  removeChildFromContainer(container, child) {
    detachChild(getContainerDomTarget(container as Container | Instance), child);
    MachiNative.removeChild(container.nativePtr, getNativePtr(child));
  },

  clearContainer(container) {
    clearDomChildren(getContainerDomTarget(container as Container | Instance));
    MachiNative.clearChildren(container.nativePtr);
    return false;
  },

  resetTextContent(instance) {
    MachiNative.updateProps(instance.nativePtr, "text", "");
  },

  commitTextUpdate(textInstance, _oldText, newText) {
    textInstance.text = newText;
    MachiNative.updateText(textInstance.nativePtr, newText);
  },

  commitUpdate(instance, updatePayload, _type, _oldProps, newProps) {
    for (const update of updatePayload) {
      if (update.kind === "event") {
        registerReactEventHandler(instance.nativePtr, update.key, update.value);
      } else {
        MachiNative.updateProps(instance.nativePtr, update.key, update.value);
      }
    }
    instance.props = newProps;
    syncHostProps(instance, newProps, collectRenderableProps(newProps));
  },

  commitMount() {},

  hideInstance(instance) {
    MachiNative.updateProps(instance.nativePtr, "visible", false);
  },

  unhideInstance(instance) {
    MachiNative.updateProps(instance.nativePtr, "visible", true);
  },

  hideTextInstance(textInstance) {
    MachiNative.updateProps(textInstance.nativePtr, "visible", false);
  },

  unhideTextInstance(textInstance) {
    MachiNative.updateProps(textInstance.nativePtr, "visible", true);
  },

  detachDeletedInstance(node) {
    if (!isTextNode(node)) {
      clearEventHandlers(node.nativePtr, node.props);
    }
    detachDeletedNode(node);
  },

  getInstanceFromNode() {
    return null;
  },

  beforeActiveInstanceBlur() {},

  afterActiveInstanceBlur() {},

  prepareScopeUpdate() {},

  getInstanceFromScope() {
    return null;
  },

  scheduleTimeout(fn, delay) {
    if (typeof setTimeout === "function") {
      return setTimeout(fn, delay) as unknown as number;
    }

    const handle = ++timeoutHandle;
    pendingTimeouts.set(handle, fn);
    scheduleMicrotaskCompat(() => {
      const pending = pendingTimeouts.get(handle);
      if (pending == null) {
        return;
      }
      pendingTimeouts.delete(handle);
      pending();
    });
    return handle;
  },

  cancelTimeout(id) {
    if (typeof clearTimeout === "function") {
      clearTimeout(id);
    }
    pendingTimeouts.delete(id);
  },

  noTimeout: -1,
  supportsMicrotasks: true,
  scheduleMicrotask: scheduleMicrotaskCompat,
  getCurrentEventPriority: () => DefaultEventPriority,
};

const MachiReconciler = Reconciler(hostConfig);

function createRoot(container?: Container | NativePtr | string) {
  const containerInfo = normalizeContainer(container);
  setDocumentRootNativePtr(containerInfo.nativePtr);
  const fiberRoot = MachiReconciler.createContainer(
    containerInfo,
    0,
    null,
    false,
    null,
    "",
    (error) => {
      console.log("[MachiRenderer] recoverable error", error);
    },
    null,
  );

  return {
    render(element: ReactNode) {
      MachiReconciler.updateContainer(element, fiberRoot, null, null);
    },
    unmount() {
      MachiReconciler.updateContainer(null, fiberRoot, null, null);
    },
    container: containerInfo,
  };
}

function render(element: ReactNode, container?: Container | NativePtr | string) {
  const root = createRoot(container);
  root.render(element);
  return root;
}

const MachiRenderer = {
  createRoot,
  render,
  NativeView,
  actions,
};

export { createRoot, render, NativeView, actions, MachiHost };
export default MachiRenderer;
