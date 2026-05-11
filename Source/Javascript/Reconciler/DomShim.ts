type MachiListener = (event: any) => void;
type ListenerStore = Map<string, Set<MachiListener>>;
type RectLike = {
  x: number;
  y: number;
  left: number;
  top: number;
  right: number;
  bottom: number;
  width: number;
  height: number;
};

const nodeRegistry = new Map<string, any>();
const targetListeners = new WeakMap<object, ListenerStore>();
const nativeElementDispatchers = new WeakMap<object, Map<string, MachiListener>>();
const reactEventHandlers = new WeakMap<object, Map<string, MachiListener>>();
const globalDispatchers = new Map<string, MachiListener>();
const timeoutCallbacks = new Map<number, () => void>();
let timeoutId = 0;
let observerId = 0;

class MachiNode {}
class MachiElement extends MachiNode {}
class MachiHTMLElement extends MachiElement {}
class MachiDocument extends MachiNode {}
class MachiEvent {
  type: string;
  defaultPrevented = false;
  cancelable = true;
  bubbles = true;
  target: any = null;
  currentTarget: any = null;
  nativeEvent: any = this;

  constructor(type: string, init: Record<string, unknown> = {}) {
    this.type = type;
    Object.assign(this, init);
  }

  preventDefault() {
    if (this.cancelable) {
      this.defaultPrevented = true;
    }
  }

  stopPropagation() {
    (this as any).__stopped = true;
  }
}

class MachiDOMRect {
  x: number;
  y: number;
  left: number;
  top: number;
  right: number;
  bottom: number;
  width: number;
  height: number;

  constructor(x = 0, y = 0, width = 0, height = 0) {
    this.x = x;
    this.y = y;
    this.left = x;
    this.top = y;
    this.width = width;
    this.height = height;
    this.right = x + width;
    this.bottom = y + height;
  }

  static fromRect(rect: Partial<RectLike> = {}) {
    return new MachiDOMRect(rect.x ?? rect.left ?? 0, rect.y ?? rect.top ?? 0, rect.width ?? 0, rect.height ?? 0);
  }
}

class MachiMutationObserver {
  private callback: (records: unknown[]) => void;

  constructor(callback: (records: unknown[]) => void) {
    this.callback = callback;
  }

  observe() {}

  disconnect() {}

  takeRecords() {
    return [];
  }
}

class MachiResizeObserver {
  private callback: (entries: unknown[]) => void;
  private observed = new Set<any>();
  private id = ++observerId;

  constructor(callback: (entries: unknown[]) => void) {
    this.callback = callback;
  }

  observe(target: any) {
    this.observed.add(target);
    const rect = target?.getBoundingClientRect?.() ?? new MachiDOMRect();
    const entry = {
      target,
      contentRect: rect,
      borderBoxSize: [{ inlineSize: rect.width, blockSize: rect.height }],
      contentBoxSize: [{ inlineSize: rect.width, blockSize: rect.height }],
    };
    const scheduleFrame = (globalThis as any).requestAnimationFrame ?? ((callback: () => void) => Promise.resolve().then(callback));
    scheduleFrame(() => {
      if (this.observed.has(target)) {
        this.callback([entry]);
      }
    });
  }

  unobserve(target: any) {
    this.observed.delete(target);
  }

  disconnect() {
    this.observed.clear();
  }
}

function createClassList(target: any) {
  const tokens = new Set<string>();
  const sync = () => {
    const value = Array.from(tokens).join(" ");
    target.attributes.class = value;
    target.className = value;
    if (target.nativePtr != null) {
      MachiNative.updateProps(target.nativePtr, "className", value);
    }
  };

  return {
    add: (...items: string[]) => {
      for (const item of items) {
        tokens.add(item);
      }
      sync();
    },
    remove: (...items: string[]) => {
      for (const item of items) {
        tokens.delete(item);
      }
      sync();
    },
    contains: (item: string) => tokens.has(item),
    toggle: (item: string, force?: boolean) => {
      const shouldAdd = force ?? !tokens.has(item);
      if (shouldAdd) {
        tokens.add(item);
      } else {
        tokens.delete(item);
      }
      sync();
      return shouldAdd;
    },
    replace: (oldToken: string, newToken: string) => {
      if (!tokens.has(oldToken)) {
        return false;
      }
      tokens.delete(oldToken);
      tokens.add(newToken);
      sync();
      return true;
    },
    toString: () => Array.from(tokens).join(" "),
  };
}

function matchesSelector(node: any, selector: string) {
  if (typeof selector !== "string" || node == null || node.nodeType !== 1) {
    return false;
  }

  const trimmed = selector.trim();
  if (trimmed.length === 0) {
    return false;
  }
  if (trimmed.startsWith("#")) {
    return node.id === trimmed.slice(1) || node.attributes?.id === trimmed.slice(1);
  }
  if (trimmed.startsWith(".")) {
    return node.classList?.contains(trimmed.slice(1)) ?? String(node.className ?? "").split(/\s+/).includes(trimmed.slice(1));
  }
  return String(node.tagName ?? node.nodeName ?? "").toLowerCase() === trimmed.toLowerCase();
}

function walkElements(root: any, visitor: (node: any) => boolean) {
  for (const child of root?.children ?? []) {
    if (visitor(child)) {
      return child;
    }
    const nested = walkElements(child, visitor);
    if (nested != null) {
      return nested;
    }
  }
  return null;
}

function collectElements(root: any, predicate: (node: any) => boolean, result: any[] = []) {
  for (const child of root?.children ?? []) {
    if (predicate(child)) {
      result.push(child);
    }
    collectElements(child, predicate, result);
  }
  return result;
}

const domToReactEvent: Record<string, string> = {
  click: "onClick",
  pointerdown: "onPointerDown",
  pointermove: "onPointerMove",
  pointerup: "onPointerUp",
  mousedown: "onMouseDown",
  mousemove: "onMouseMove",
  mouseup: "onMouseUp",
  keydown: "onKeyDown",
  keyup: "onKeyUp",
  dragstart: "onDragStart",
  drag: "onDrag",
  dragend: "onDragEnd",
};

const reactToDomEvent = Object.fromEntries(Object.entries(domToReactEvent).map(([domType, reactName]) => [reactName, domType]));

const nativeToDomType: Record<string, string> = {
  pointerDown: "pointerdown",
  pointerMove: "pointermove",
  pointerUp: "pointerup",
  keyDown: "keydown",
  keyUp: "keyup",
  windowResize: "resize",
  windowFocus: "focus",
  windowBlur: "blur",
  windowClose: "close",
};

function ptrKey(ptr: NativePtr) {
  return String(ptr);
}

function lowerType(type: string) {
  return nativeToDomType[type] ?? type.toLowerCase();
}

function getListeners(target: object) {
  let listeners = targetListeners.get(target);
  if (listeners == null) {
    listeners = new Map();
    targetListeners.set(target, listeners);
  }
  return listeners;
}

function addListener(target: object, type: string, listener: MachiListener) {
  const normalizedType = lowerType(type);
  const listeners = getListeners(target);
  let typedListeners = listeners.get(normalizedType);
  if (typedListeners == null) {
    typedListeners = new Set();
    listeners.set(normalizedType, typedListeners);
  }
  typedListeners.add(listener);
}

function removeListener(target: object, type: string, listener: MachiListener) {
  const typedListeners = targetListeners.get(target)?.get(lowerType(type));
  typedListeners?.delete(listener);
}

function hasDomListeners(target: object, type: string) {
  return (targetListeners.get(target)?.get(lowerType(type))?.size ?? 0) > 0;
}

function hasReactHandler(target: object, nativeEventName: string) {
  return reactEventHandlers.get(target)?.has(nativeEventName) ?? false;
}

function dispatchToTarget(target: object, event: any) {
  const listeners = targetListeners.get(target)?.get(lowerType(event.type));
  if (listeners == null) {
    return true;
  }

  for (const listener of Array.from(listeners)) {
    listener(event);
    if (event.__stopped) {
      break;
    }
  }
  return !event.defaultPrevented;
}

function installEventTarget(target: any) {
  target.addEventListener = (type: string, listener: MachiListener) => {
    if (typeof listener !== "function") {
      return;
    }
    addListener(target, type, listener);
    if (target === documentShim || target === windowShim) {
      ensureGlobalDispatcher(lowerType(type));
    } else {
      ensureNativeElementDispatcher(target, lowerType(type));
    }
  };

  target.removeEventListener = (type: string, listener: MachiListener) => {
    const normalizedType = lowerType(type);
    removeListener(target, normalizedType, listener);
    cleanupNativeElementDispatcher(target, normalizedType);
  };

  target.dispatchEvent = (event: any) => {
    const normalized = normalizeNativeEvent(event, target);
    if (normalized.currentTarget == null) {
      normalized.currentTarget = target;
    }
    return dispatchToTarget(target, normalized);
  };

  return target;
}

function createFakeElement(name: string) {
  const element = Object.create(MachiHTMLElement.prototype);
  element.nodeType = 1;
  element.nodeName = name.toUpperCase();
  element.tagName = name.toUpperCase();
  element.childNodes = [];
  element.children = [];
  element.style = {};
  element.attributes = {};
  element.dataset = {};
  element.className = "";
  element.classList = createClassList(element);
  element.isConnected = true;
  element.contains = (node: any) => containsNode(element, node);
  element.appendChild = (child: any) => {
    attachChild(element, child);
    return child;
  };
  element.removeChild = (child: any) => {
    detachChild(element, child);
    return child;
  };
  element.insertBefore = (child: any, beforeChild: any) => {
    insertChildBefore(element, child, beforeChild);
    return child;
  };
  element.matches = (selector: string) => matchesSelector(element, selector);
  element.closest = (selector: string) => {
    for (let current = element; current != null; current = current.parentElement) {
      if (matchesSelector(current, selector)) {
        return current;
      }
    }
    return null;
  };
  element.querySelector = (selector: string) => walkElements(element, (node) => matchesSelector(node, selector));
  element.querySelectorAll = (selector: string) => collectElements(element, (node) => matchesSelector(node, selector));
  element.setPointerCapture = () => {};
  element.releasePointerCapture = () => {};
  element.hasPointerCapture = () => false;
  element.getBoundingClientRect = () => new MachiDOMRect();
  defineTreeAccessors(element);
  return installEventTarget(element);
}

function defineTreeAccessors(target: any) {
  Object.defineProperty(target, "firstChild", {
    configurable: true,
    get() {
      return target.childNodes?.[0] ?? null;
    },
  });
  Object.defineProperty(target, "lastChild", {
    configurable: true,
    get() {
      return target.childNodes?.[target.childNodes.length - 1] ?? null;
    },
  });
  Object.defineProperty(target, "firstElementChild", {
    configurable: true,
    get() {
      return target.children?.[0] ?? null;
    },
  });
  Object.defineProperty(target, "lastElementChild", {
    configurable: true,
    get() {
      return target.children?.[target.children.length - 1] ?? null;
    },
  });
}

function createStyleProxy(nativePtr: NativePtr) {
  const values: Record<string, unknown> = {};
  return new Proxy(values, {
    set(target, property, value) {
      const key = String(property);
      target[key] = value;
      MachiNative.updateProps(nativePtr, key, value);
      return true;
    },
    get(target, property) {
      if (property === "setProperty") {
        return (key: string, value: unknown) => {
          target[key] = value;
          MachiNative.updateProps(nativePtr, key, value);
        };
      }
      if (property === "getPropertyValue") {
        return (key: string) => target[key] ?? "";
      }
      if (property === "removeProperty") {
        return (key: string) => {
          const previous = target[key];
          delete target[key];
          MachiNative.updateProps(nativePtr, key, null);
          return previous ?? "";
        };
      }
      return target[String(property)];
    },
  });
}

const windowShim: any = installEventTarget({});
const documentShim: any = installEventTarget(Object.create(MachiDocument.prototype));
const documentElement = createFakeElement("html");
const documentBody = createFakeElement("body");

documentElement.ownerDocument = documentShim;
documentBody.ownerDocument = documentShim;
documentBody.parentNode = documentElement;
documentBody.parentElement = documentElement;
documentElement.childNodes.push(documentBody);
documentElement.children.push(documentBody);

documentShim.nodeType = 9;
documentShim.nodeName = "#document";
documentShim.defaultView = windowShim;
documentShim.documentElement = documentElement;
documentShim.body = documentBody;
documentShim.activeElement = null;
documentShim.contains = (node: any) => containsNode(documentElement, node);
documentShim.createElement = (type: string) => createFakeElement(type);
documentShim.createElementNS = (_namespace: string, type: string) => createFakeElement(type);
documentShim.createTextNode = (text: string) => ({
  nodeType: 3,
  nodeName: "#text",
  nodeValue: text,
  textContent: text,
  ownerDocument: documentShim,
  parentNode: null,
});
documentShim.addEventListener = documentShim.addEventListener;
documentShim.querySelector = (selector: string) => documentElement.querySelector(selector);
documentShim.querySelectorAll = (selector: string) => documentElement.querySelectorAll(selector);
documentShim.elementFromPoint = (x: number, y: number) => {
  for (const node of Array.from(nodeRegistry.values()).reverse()) {
    const rect = node.getBoundingClientRect?.();
    if (rect != null && x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom) {
      return node;
    }
  }
  return null;
};

windowShim.window = windowShim;
windowShim.self = windowShim;
windowShim.globalThis = globalThis;
windowShim.document = documentShim;
windowShim.navigator = { userAgent: "MachiUI" };
windowShim.getComputedStyle = (node: any) => ({
  ...node?.style,
  getPropertyValue(property: string) {
    const camelKey = property.replace(/-([a-z])/g, (_, character: string) => character.toUpperCase());
    return node?.style?.[camelKey] ?? node?.style?.[property] ?? "";
  },
});

function installDomGlobals() {
  const root = globalThis as any;
  root.window = windowShim;
  root.self = windowShim;
  root.document = documentShim;
  root.navigator = windowShim.navigator;
  root.Node = root.Node ?? MachiNode;
  root.Element = root.Element ?? MachiElement;
  root.HTMLElement = root.HTMLElement ?? MachiHTMLElement;
  root.Document = root.Document ?? MachiDocument;
  root.Event = root.Event ?? MachiEvent;
  root.MouseEvent = root.MouseEvent ?? MachiEvent;
  root.PointerEvent = root.PointerEvent ?? MachiEvent;
  root.KeyboardEvent = root.KeyboardEvent ?? MachiEvent;
  root.DOMRect = root.DOMRect ?? MachiDOMRect;
  root.MutationObserver = root.MutationObserver ?? MachiMutationObserver;
  root.ResizeObserver = root.ResizeObserver ?? MachiResizeObserver;
  root.performance = root.performance ?? { now: () => Date.now() };
  root.setTimeout = root.setTimeout ?? ((callback: () => void) => {
    const id = ++timeoutId;
    timeoutCallbacks.set(id, callback);
    Promise.resolve().then(() => {
      const pending = timeoutCallbacks.get(id);
      if (pending == null) {
        return;
      }
      timeoutCallbacks.delete(id);
      pending();
    });
    return id;
  });
  root.clearTimeout = root.clearTimeout ?? ((id: number) => {
    timeoutCallbacks.delete(id);
  });
  root.requestAnimationFrame = root.requestAnimationFrame ?? ((callback: (time: number) => void) => {
    return root.setTimeout(() => callback(root.performance.now()), 16);
  });
  root.cancelAnimationFrame = root.cancelAnimationFrame ?? root.clearTimeout;

  windowShim.Node = root.Node;
  windowShim.Element = root.Element;
  windowShim.HTMLElement = root.HTMLElement;
  windowShim.Document = root.Document;
  windowShim.Event = root.Event;
  windowShim.MouseEvent = root.MouseEvent;
  windowShim.PointerEvent = root.PointerEvent;
  windowShim.KeyboardEvent = root.KeyboardEvent;
  windowShim.DOMRect = root.DOMRect;
  windowShim.MutationObserver = root.MutationObserver;
  windowShim.ResizeObserver = root.ResizeObserver;
  windowShim.performance = root.performance;
  windowShim.setTimeout = root.setTimeout;
  windowShim.clearTimeout = root.clearTimeout;
  windowShim.requestAnimationFrame = root.requestAnimationFrame;
  windowShim.cancelAnimationFrame = root.cancelAnimationFrame;
}

function getNodeByPtr(ptr: unknown) {
  if (ptr == null || typeof ptr === "object") {
    return ptr;
  }
  return nodeRegistry.get(String(ptr)) ?? null;
}

function createEventPath(target: any) {
  const path = [];
  for (let current = target; current != null; current = current.parentNode) {
    path.push(current);
  }
  path.push(documentShim, windowShim);
  return path;
}

function normalizeNativeEvent(nativeEvent: any, fallbackTarget?: any) {
  const event = nativeEvent == null || typeof nativeEvent !== "object"
    ? new MachiEvent(String(nativeEvent ?? "event"))
    : nativeEvent;
  event.type = lowerType(String(event.type ?? "event"));

  const target = getNodeByPtr(event.target) ?? fallbackTarget ?? documentBody;
  const currentTarget = getNodeByPtr(event.currentTarget) ?? fallbackTarget ?? target;
  event.target = target;
  event.currentTarget = currentTarget;
  event.nativeEvent = event;
  event.isPrimary = event.isPrimary ?? true;
  event.cancelable = event.cancelable ?? true;
  event.bubbles = event.bubbles ?? true;
  event.defaultPrevented = event.defaultPrevented ?? false;
  event.preventDefault = event.preventDefault ?? (() => {
    if (event.cancelable) {
      event.defaultPrevented = true;
    }
  });
  event.stopPropagation = event.stopPropagation ?? (() => {
    event.__stopped = true;
  });
  event.composedPath = event.composedPath ?? (() => createEventPath(target));
  return event;
}

function containsNode(parent: any, node: any): boolean {
  if (parent == null || node == null) {
    return false;
  }
  for (let current = node; current != null; current = current.parentNode) {
    if (current === parent) {
      return true;
    }
  }
  return false;
}

function ensureGlobalDispatcher(type: string) {
  if (globalDispatchers.has(type)) {
    return;
  }

  const dispatcher = (nativeEvent: any) => {
    const event = normalizeNativeEvent(nativeEvent);
    if (type === "resize" || type === "focus" || type === "blur" || type === "close") {
      dispatchToTarget(windowShim, event);
    }
    dispatchToTarget(documentShim, event);
  };

  globalDispatchers.set(type, dispatcher);
  MachiNative.updateGlobalEventHandler(type, dispatcher);
}

function ensureNativeElementDispatcher(target: any, type: string) {
  const nativeEventName = domToReactEvent[type];
  if (nativeEventName == null || target?.nativePtr == null) {
    return;
  }

  let dispatchers = nativeElementDispatchers.get(target);
  if (dispatchers == null) {
    dispatchers = new Map();
    nativeElementDispatchers.set(target, dispatchers);
  }
  if (dispatchers.has(type)) {
    return;
  }

  const dispatcher = (nativeEvent: any) => {
    const event = normalizeNativeEvent(nativeEvent, target);
    const reactHandler = reactEventHandlers.get(target)?.get(nativeEventName);
    if (reactHandler != null) {
      reactHandler(event);
    }
    dispatchToTarget(target, event);
  };
  dispatchers.set(type, dispatcher);
  MachiNative.updateEventHandler(target.nativePtr, nativeEventName, dispatcher);
}

function cleanupNativeElementDispatcher(target: any, type: string) {
  const nativeEventName = domToReactEvent[type];
  if (nativeEventName == null || target?.nativePtr == null) {
    return;
  }
  if (hasDomListeners(target, type) || hasReactHandler(target, nativeEventName)) {
    return;
  }

  nativeElementDispatchers.get(target)?.delete(type);
  MachiNative.updateEventHandler(target.nativePtr, nativeEventName, null);
}

export function createHostInstance(type: string, nativePtr: NativePtr, props: Props): Instance {
  const instance: any = Object.create(MachiHTMLElement.prototype);
  instance.type = type;
  instance.nativePtr = nativePtr;
  instance.props = props;
  instance.nodeType = 1;
  instance.nodeName = type.toUpperCase();
  instance.tagName = type.toUpperCase();
  instance.ownerDocument = documentShim;
  instance.parentNode = null;
  instance.parentElement = null;
  instance.childNodes = [];
  instance.children = [];
  instance.attributes = {};
  instance.dataset = {};
  instance.className = "";
  instance.id = "";
  instance.classList = createClassList(instance);
  instance.style = createStyleProxy(nativePtr);
  instance.isConnected = true;
  instance.contains = (node: any) => containsNode(instance, node);
  instance.appendChild = (child: HostNode) => {
    attachChild(instance, child);
    MachiNative.appendChild(nativePtr, child.nativePtr);
    return child;
  };
  instance.removeChild = (child: HostNode) => {
    detachChild(instance, child);
    MachiNative.removeChild(nativePtr, child.nativePtr);
    return child;
  };
  instance.insertBefore = (child: HostNode, beforeChild: HostNode) => {
    insertChildBefore(instance, child, beforeChild);
    MachiNative.insertBefore(nativePtr, child.nativePtr, beforeChild.nativePtr);
    return child;
  };
  instance.replaceChildren = (...children: HostNode[]) => {
    clearChildren(instance);
    MachiNative.clearChildren(nativePtr);
    for (const child of children) {
      instance.appendChild(child);
    }
  };
  instance.matches = (selector: string) => matchesSelector(instance, selector);
  instance.closest = (selector: string) => {
    for (let current = instance; current != null; current = current.parentElement) {
      if (matchesSelector(current, selector)) {
        return current;
      }
    }
    return null;
  };
  instance.querySelector = (selector: string) => walkElements(instance, (node) => matchesSelector(node, selector));
  instance.querySelectorAll = (selector: string) => collectElements(instance, (node) => matchesSelector(node, selector));
  instance.setPointerCapture = () => {};
  instance.releasePointerCapture = () => {};
  instance.hasPointerCapture = () => false;
  defineTreeAccessors(instance);
  instance.focus = () => {
    documentShim.activeElement = instance;
  };
  instance.blur = () => {
    if (documentShim.activeElement === instance) {
      documentShim.activeElement = null;
    }
  };
  instance.setAttribute = (key: string, value: unknown) => {
    instance.attributes[key] = String(value);
    if (key === "id") {
      instance.id = String(value);
    }
    if (key === "class" || key === "className") {
      instance.className = String(value);
      for (const token of String(value).split(/\s+/)) {
        if (token.length > 0) {
          instance.classList.add(token);
        }
      }
    }
    if (key.startsWith("data-")) {
      const dataKey = key.slice(5).replace(/-([a-z])/g, (_, character: string) => character.toUpperCase());
      instance.dataset[dataKey] = String(value);
    }
    MachiNative.updateProps(nativePtr, key, value);
  };
  instance.getAttribute = (key: string) => instance.attributes[key] ?? null;
  instance.hasAttribute = (key: string) => instance.attributes[key] != null;
  instance.removeAttribute = (key: string) => {
    delete instance.attributes[key];
    MachiNative.updateProps(nativePtr, key, null);
  };
  instance.getBoundingClientRect = () => {
    const rect = MachiNative.getBoundingClientRect(nativePtr);
    return MachiDOMRect.fromRect(rect);
  };

  installEventTarget(instance);
  syncHostProps(instance, props, {});
  nodeRegistry.set(ptrKey(nativePtr), instance);
  return instance as Instance;
}

export function createHostTextInstance(nativePtr: NativePtr, text: string): TextInstance {
  const instance: any = Object.create(MachiNode.prototype);
  instance.type = "#text";
  instance.nativePtr = nativePtr;
  instance.text = text;
  instance.nodeType = 3;
  instance.nodeName = "#text";
  instance.nodeValue = text;
  instance.parentNode = null;
  instance.ownerDocument = documentShim;
  nodeRegistry.set(ptrKey(nativePtr), instance);
  return instance as TextInstance;
}

export function syncHostProps(instance: Instance, props: Props, renderableProps: Record<string, unknown>) {
  const target = instance as any;
  target.props = props;
  target.attributes ??= {};
  target.style ??= {};
  if (props.className != null) {
    target.className = String(props.className);
    target.attributes.class = target.className;
  }

  for (const key of Object.keys(renderableProps)) {
    target.attributes[key] = renderableProps[key];
    target.style[key] = renderableProps[key];
    if (key === "id") {
      target.id = String(renderableProps[key] ?? "");
    }
    if (key === "className" || key === "class") {
      target.className = String(renderableProps[key] ?? "");
      target.attributes.class = target.className;
    }
    if (key.startsWith("data-")) {
      const dataKey = key.slice(5).replace(/-([a-z])/g, (_: string, character: string) => character.toUpperCase());
      target.dataset[dataKey] = String(renderableProps[key] ?? "");
    }
  }
}

export function registerReactEventHandler(nativePtr: NativePtr, key: string, value: unknown) {
  const target = getNodeByPtr(nativePtr);
  if (target == null || typeof target !== "object") {
    if (typeof value !== "function") {
      MachiNative.updateEventHandler(nativePtr, key, null);
      return;
    }

    const dispatcher = (nativeEvent: unknown) => {
      const event = normalizeNativeEvent(nativeEvent);
      (value as MachiListener)(event);
    };
    MachiNative.updateEventHandler(nativePtr, key, dispatcher);
    return;
  }

  let handlers = reactEventHandlers.get(target);
  if (handlers == null) {
    handlers = new Map();
    reactEventHandlers.set(target, handlers);
  }

  if (typeof value === "function") {
    handlers.set(key, value as MachiListener);
    const domType = reactToDomEvent[key];
    if (domType != null) {
      ensureNativeElementDispatcher(target, domType);
    }
  } else {
    handlers.delete(key);
    const domType = reactToDomEvent[key];
    if (domType != null) {
      cleanupNativeElementDispatcher(target, domType);
    }
  }
}

export function attachChild(parent: any, child: HostNode) {
  const target = child as any;
  if (target.parentNode != null) {
    detachChild(target.parentNode, child);
  }

  target.parentNode = parent;
  target.parentElement = parent?.nodeType === 1 ? parent : null;
  parent.childNodes ??= [];
  parent.children ??= [];
  parent.childNodes.push(child);
  if ((child as any).nodeType === 1) {
    parent.children.push(child);
  }
}

export function insertChildBefore(parent: any, child: HostNode, beforeChild: HostNode) {
  attachChild(parent, child);
  const childNodes = parent.childNodes ?? [];
  const children = parent.children ?? [];
  const oldIndex = childNodes.indexOf(child);
  const beforeIndex = childNodes.indexOf(beforeChild);
  if (oldIndex >= 0 && beforeIndex >= 0) {
    childNodes.splice(oldIndex, 1);
    childNodes.splice(beforeIndex, 0, child);
  }

  if ((child as any).nodeType === 1) {
    const oldChildIndex = children.indexOf(child);
    const beforeChildIndex = children.indexOf(beforeChild);
    if (oldChildIndex >= 0 && beforeChildIndex >= 0) {
      children.splice(oldChildIndex, 1);
      children.splice(beforeChildIndex, 0, child);
    }
  }
}

export function detachChild(parent: any, child: HostNode) {
  if (parent?.childNodes != null) {
    const index = parent.childNodes.indexOf(child);
    if (index >= 0) {
      parent.childNodes.splice(index, 1);
    }
  }
  if (parent?.children != null) {
    const index = parent.children.indexOf(child);
    if (index >= 0) {
      parent.children.splice(index, 1);
    }
  }
  (child as any).parentNode = null;
  (child as any).parentElement = null;
}

export function clearChildren(parent: any) {
  for (const child of Array.from(parent.childNodes ?? [])) {
    detachChild(parent, child as HostNode);
  }
}

export function detachDeletedNode(node: HostNode) {
  nodeRegistry.delete(ptrKey(node.nativePtr));
  const target = node as any;
  const dispatchers = nativeElementDispatchers.get(target);
  if (dispatchers != null) {
    for (const type of dispatchers.keys()) {
      const nativeEventName = domToReactEvent[type];
      if (nativeEventName != null) {
        MachiNative.updateEventHandler(node.nativePtr, nativeEventName, null);
      }
    }
    dispatchers.clear();
  }
  reactEventHandlers.delete(target);
  targetListeners.delete(target);
  if (target.parentNode != null) {
    detachChild(target.parentNode, node);
  }
}

export function attachToDocument(child: HostNode) {
  attachChild(documentBody, child);
}

export function getDocumentBody() {
  return documentBody;
}

export function setDocumentRootNativePtr(nativePtr: NativePtr) {
  documentBody.nativePtr = nativePtr;
}

installDomGlobals();
