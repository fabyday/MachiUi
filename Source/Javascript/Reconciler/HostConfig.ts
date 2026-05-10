import Reconciler from "react-reconciler";
import type { HostConfig } from "react-reconciler";
import type { ReactNode } from "react";
import { DefaultEventPriority } from "react-reconciler/constants";

type HostContext = null;
type PropUpdate = {
  key: string;
  value: unknown;
};
type UpdatePayload = PropUpdate[];
type TimeoutHandle = number;
type NoTimeout = -1;

const INTERNAL_PROPS = new Set(["children", "key", "ref"]);
const pendingTimeouts = new Map<number, () => unknown>();
let timeoutHandle = 0;

function isTextNode(node: HostNode): node is TextInstance {
  return node.type === "#text";
}

function getNativePtr(node: HostNode): NativePtr {
  return node.nativePtr;
}

function forEachRenderableProp(
  props: Props,
  callback: (key: string, value: unknown) => void,
) {
  for (const key of Object.keys(props)) {
    if (INTERNAL_PROPS.has(key)) {
      continue;
    }

    const value = props[key];
    if (key === "style" && value != null && typeof value === "object") {
      for (const styleKey of Object.keys(value as Record<string, unknown>)) {
        callback(styleKey, (value as Record<string, unknown>)[styleKey]);
      }
      continue;
    }

    if (typeof value !== "function") {
      callback(key, value);
    }
  }
}

function applyProps(nativePtr: NativePtr, props: Props) {
  forEachRenderableProp(props, (key, value) => {
    MachiNative.updateProps(nativePtr, key, value);
  });
}

function diffProps(oldProps: Props, newProps: Props): UpdatePayload | null {
  const updates: UpdatePayload = [];
  const oldKeys = new Set<string>();

  forEachRenderableProp(oldProps, (key) => {
    oldKeys.add(key);
  });

  forEachRenderableProp(newProps, (key, value) => {
    oldKeys.delete(key);
    const oldValue =
      key in oldProps
        ? oldProps[key]
        : typeof oldProps.style === "object" && oldProps.style != null
          ? (oldProps.style as Record<string, unknown>)[key]
          : undefined;

    if (oldValue !== value) {
      updates.push({ key, value });
    }
  });

  for (const removedKey of oldKeys) {
    updates.push({ key: removedKey, value: null });
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

    return {
      type,
      nativePtr,
      props,
    };
  },

  createTextInstance(text) {
    return {
      type: "#text",
      nativePtr: MachiNative.createTextNode(text),
      text,
    };
  },

  appendInitialChild(parent, child) {
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
    MachiNative.appendChild(parent.nativePtr, getNativePtr(child));
  },

  appendChildToContainer(container, child) {
    MachiNative.appendChild(container.nativePtr, getNativePtr(child));
  },

  insertBefore(parent, child, beforeChild) {
    MachiNative.insertBefore(parent.nativePtr, getNativePtr(child), getNativePtr(beforeChild));
  },

  insertInContainerBefore(container, child, beforeChild) {
    MachiNative.insertBefore(container.nativePtr, getNativePtr(child), getNativePtr(beforeChild));
  },

  removeChild(parent, child) {
    MachiNative.removeChild(parent.nativePtr, getNativePtr(child));
  },

  removeChildFromContainer(container, child) {
    MachiNative.removeChild(container.nativePtr, getNativePtr(child));
  },

  clearContainer(container) {
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
      MachiNative.updateProps(instance.nativePtr, update.key, update.value);
    }
    instance.props = newProps;
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

  detachDeletedInstance() {},

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
};

export { createRoot, render };
export default MachiRenderer;
