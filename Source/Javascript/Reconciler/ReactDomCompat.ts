import MachiRenderer, { createRoot as createMachiRoot, render as renderMachi } from "./HostConfig";

const ReactPortalType = Symbol.for("react.portal");

function normalizePortalContainer(container: any) {
  if (container?.nativePtr != null) {
    return container;
  }

  const body = (globalThis as any).document?.body;
  if (body?.nativePtr != null) {
    return body;
  }

  return container;
}

export function createPortal(children: unknown, container: unknown, key: null | string = null) {
  return {
    $$typeof: ReactPortalType,
    key: key == null ? null : String(key),
    children,
    containerInfo: normalizePortalContainer(container),
    implementation: null,
  };
}

export function createRoot(container?: Container | NativePtr | string) {
  return createMachiRoot(container);
}

export function hydrateRoot(container: Container | NativePtr | string, children: unknown) {
  const root = createMachiRoot(container);
  root.render(children as never);
  return root;
}

export function render(element: unknown, container?: Container | NativePtr | string) {
  return renderMachi(element as never, container);
}

export function unmountComponentAtNode(container?: Container | NativePtr | string) {
  const root = createMachiRoot(container);
  root.unmount();
  return true;
}

export function flushSync<T>(callback: () => T): T {
  return callback();
}

export function unstable_batchedUpdates<T>(callback: () => T): T {
  return callback();
}

export function findDOMNode(node: unknown) {
  if (node == null) {
    return null;
  }
  if (typeof node === "object" && "nativePtr" in node) {
    return node;
  }
  return null;
}

export const version = "18.3.1-machi";

const ReactDomCompat = {
  createPortal,
  createRoot,
  hydrateRoot,
  render,
  unmountComponentAtNode,
  flushSync,
  unstable_batchedUpdates,
  findDOMNode,
  version,
  default: undefined as unknown,
};

ReactDomCompat.default = ReactDomCompat;

export default ReactDomCompat;
