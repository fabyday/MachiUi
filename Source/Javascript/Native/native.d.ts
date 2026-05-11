export {};

declare global {
  type NativePtr = number | bigint;

  interface MachiNative {
    createRoot(sceneName?: string): NativePtr;
    createElement(type: string): NativePtr;
    createTextNode(text: string): NativePtr;
    appendChild(parentPtr: NativePtr, childPtr: NativePtr): void;
    insertBefore(parentPtr: NativePtr, childPtr: NativePtr, beforeChildPtr: NativePtr): void;
    removeChild(parentPtr: NativePtr, childPtr: NativePtr): void;
    clearChildren(parentPtr: NativePtr): void;
    updateProps(ptr: NativePtr, key: string, value: unknown): void;
    updateEventHandler(ptr: NativePtr, key: string, value: unknown): void;
    updateGlobalEventHandler(key: string, value: unknown): void;
    getBoundingClientRect(ptr: NativePtr): {
      x: number;
      y: number;
      left: number;
      top: number;
      right: number;
      bottom: number;
      width: number;
      height: number;
    };
    updateText(ptr: NativePtr, text: string): void;
    isNetworkEnabled(): boolean;
    fetchSync(url: string, init?: {
      method?: string;
      headers?: Record<string, string>;
      body?: string;
    }): {
      ok: boolean;
      status: number;
      statusText: string;
      url: string;
      body: string;
      headers: Array<[string, string]>;
    };
    invokeAction(name: string, payloadJson?: string): {
      ok: boolean;
      payload: string;
      error: string;
    };
  }

  var MachiNative: MachiNative;

  var MachiRuntime: {
    readonly capabilities: {
      readonly network: boolean;
    };
    hasCapability(name: string): boolean;
  };

  var Machi: {
    actions: {
      invoke(name: string, payload?: unknown): Promise<unknown>;
      invokeSync(name: string, payload?: unknown): unknown;
    };
  };

  type MachiPointerEvent = {
    type: string;
    target: NativePtr;
    currentTarget: NativePtr;
    x: number;
    y: number;
    clientX: number;
    clientY: number;
    deltaX: number;
    deltaY: number;
    button: number;
    buttons: number;
    nativeEvent?: MachiPointerEvent;
    preventDefault?: () => void;
    stopPropagation?: () => void;
  };

  type MachiKeyEvent = {
    type: string;
    target: NativePtr;
    currentTarget: NativePtr;
    keyCode: number;
    which: number;
    altKey: boolean;
    ctrlKey: boolean;
    shiftKey: boolean;
    metaKey: boolean;
    nativeEvent?: MachiKeyEvent;
    preventDefault?: () => void;
    stopPropagation?: () => void;
  };

  type MachiWindowEvent = {
    type: string;
    target: NativePtr;
    currentTarget: NativePtr;
    width: number;
    height: number;
    nativeEvent?: MachiWindowEvent;
    preventDefault?: () => void;
    stopPropagation?: () => void;
  };

  type Props = Record<string, unknown> & {
    children?: unknown;
    className?: string;
    style?: Record<string, unknown>;
    onClick?: (event: MachiPointerEvent) => void;
    onPointerDown?: (event: MachiPointerEvent) => void;
    onPointerMove?: (event: MachiPointerEvent) => void;
    onPointerUp?: (event: MachiPointerEvent) => void;
    onMouseDown?: (event: MachiPointerEvent) => void;
    onMouseMove?: (event: MachiPointerEvent) => void;
    onMouseUp?: (event: MachiPointerEvent) => void;
    onDragStart?: (event: MachiPointerEvent) => void;
    onDrag?: (event: MachiPointerEvent) => void;
    onDragEnd?: (event: MachiPointerEvent) => void;
    onKeyDown?: (event: MachiKeyEvent) => void;
    onKeyUp?: (event: MachiKeyEvent) => void;
    onWindowResize?: (event: MachiWindowEvent) => void;
    onWindowFocus?: (event: MachiWindowEvent) => void;
    onWindowBlur?: (event: MachiWindowEvent) => void;
    onWindowClose?: (event: MachiWindowEvent) => void;
  };

  namespace JSX {
    interface IntrinsicElements {
      "native-view": Props & {
        nativeViewType?: string;
        viewType?: string;
        viewId?: string;
      };
    }
  }

  type Instance = {
    type: string;
    nativePtr: NativePtr;
    props: Props;
    nodeType?: number;
    nodeName?: string;
    tagName?: string;
    ownerDocument?: unknown;
    parentNode?: unknown;
    parentElement?: unknown;
    childNodes?: HostNode[];
    children?: Instance[];
    style?: Record<string, unknown>;
    addEventListener?: (type: string, listener: (event: unknown) => void) => void;
    removeEventListener?: (type: string, listener: (event: unknown) => void) => void;
    dispatchEvent?: (event: unknown) => boolean;
    getBoundingClientRect?: () => {
      x: number;
      y: number;
      left: number;
      top: number;
      right: number;
      bottom: number;
      width: number;
      height: number;
    };
    contains?: (node: unknown) => boolean;
  };

  type TextInstance = {
    type: "#text";
    nativePtr: NativePtr;
    text: string;
  };

  type HostNode = Instance | TextInstance;

  type Container = {
    rootTag: string;
    nativePtr: NativePtr;
  };
}
