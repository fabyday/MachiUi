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
    updateText(ptr: NativePtr, text: string): void;
  }

  var MachiNative: MachiNative;

  type Props = Record<string, unknown> & {
    children?: unknown;
    style?: Record<string, unknown>;
  };

  type Instance = {
    type: string;
    nativePtr: NativePtr;
    props: Props;
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
