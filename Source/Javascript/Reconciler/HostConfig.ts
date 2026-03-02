import Reconciler, { HostConfig } from "react-reconciler";
import {
  DiscreteEventPriority,
  ContinuousEventPriority,
  DefaultEventPriority,
} from "react-reconciler/constants";
// C++에서 주입해줄 전역 객체 (MachiNative) 정의

const hostConfig: HostConfig<
  string, // Type
  Props, // Props
  Container, // Container
  Instance, // Instance
  TextInstance, // TextInstance
  never, // SuspenseInstance
  never, // HydratableInstance
  Instance, // PublicInstance
  any, // HostContext
  boolean, // UpdatePayload (변경 사항 유무)
  never, // ChildSet
  number, // TimeoutHandle
  number // NoTimeout>= {
> = {
  // 1. 요소가 처음 만들어질 때 (예: <button />)
  createInstance(
    type: string,
    props: any,
    rootContainer: any,
    hostContext: any,
    internalHandle: any,
  ) {
    // C++로 넘어가서 실제 객체를 만들고 그 주소를 받아옴
    const nativePtr = MachiNative.createElement(type, props);

    return {
      type,
      nativePtr, // 이 ID로 나중에 C++ 객체를 찾아 조작함
      props,
    };
  },
  createTextInstance(
    text: any,
    rootContainer: any,
    hostContext: any,
    internalHandle: any,
  ) {
    return text;
  },
  appendInitialChild(parentInstance, child) {},
  finalizeInitialChildren(instance, type, props, rootContainer, hostContext) {
    return false;
  },
  shouldSetTextContent(type, props) {
    return false;
  },

  getRootHostContext(rootContainer) {},
  getChildHostContext(parentHostContext, type, rootContainer) {},
  getPublicInstance(instance: Instance | TextInstance): Instance {
    // 텍스트 인스턴스는 PublicInstance가 될 수 없으므로
    // 타입 시스템 상에서 Instance로 간주하게 합니다.
    return instance as Instance;
  },
  prepareForCommit(containerInfo: Container): Record<string, any> | null {
    // 아무런 부가 데이터도 필요 없으므로 null을 리턴합니다.
    return null;
  },
  prepareUpdate(
    instance: Instance,
    type: string,
    oldProps: any,
    newProps: any,
    rootContainer: Container,
    hostContext: any,
  ): boolean | null {
    // 단순하게 true를 반환하면 항상 업데이트를 시도합니다.
    return true;
  },
  getInstanceFromScope(scopeInstance: any) {
    // 스코프에서 인스턴스를 찾아오는 로직
    return null;
  },
  // Scope API 관련 필수 구현 (사용하지 않으므로 비워둠)
  prepareScopeUpdate(scopeInstance: any, instance: any) {
    // 스코프 업데이트 로직 (미구현)
  },

  beforeActiveInstanceBlur() {
    // 포커스가 나가기 전 처리
  },

  afterActiveInstanceBlur() {
    // 포커스가 나간 후 처리
  },

  detachDeletedInstance(instance: Instance) {
    // 삭제된 인스턴스 정리
  },
  resetAfterCommit(containerInfo) {},
  preparePortalMount(containerInfo) {},
  scheduleTimeout(fn: (...args: unknown[]) => unknown, delay?: number): number {
    // setTimeout은 타이머 ID(number)를 반환합니다.
    return setTimeout(fn, delay) as unknown as number;
  },

  cancelTimeout(id: number): void {
    // 나중에 리액트가 이 ID를 들고 와서 취소를 요청합니다.
    clearTimeout(id);
  },

  // This is a property (not a function) that should be set to something that can never be a valid timeout ID. For example, you can set it to -1.
  noTimeout: -1,

  // 3. 트리 탐색 및 기타 (대부분 비워둠)
  getInstanceFromNode(node: any) {
    return null;
  },

  // 4. 컨테이너 관리
  clearContainer(container: Container) {
    // 루트를 비울 때 호출됩니다. 필요 시 C++에 알림을 보냅니다.
  },

  // 5. 마이크로태스크 (최신 리컨실러 필수)
  scheduleMicrotask(fn: any) {
    queueMicrotask(fn);
  },
  supportsMicrotasks: true,
  getCurrentEventPriority: () => DiscreteEventPriority,

  // MUTATION Methods
  // 2. 부모-자식 관계 연결
  appendChild(parent: any, child: any) {
    MachiNative.appendChild(parent.nativePtr, child.nativePtr);
  },
  appendChildToContainer(container: any, child: any) {
    MachiNative.appendChild(container.nativePtr, child.nativePtr);
  },
  insertBefore(parent: any, child: any, beforeChild: any) {},
  insertInContainerBefore(container: any, child: any, beforeChild: any) {},
  removeChild(parent: any, child: any) {},
  removeChildFromContainer(container: any, child: any) {},
  resetTextContent(instance: any) {},
  commitTextUpdate(textInstance: any, oldText: any, newText: any) {},
  commitMount(instance, type, props, internalHandle) {},

  // 3. 속성 변경 (텍스트 변경, 색상 변경 등)
  commitUpdate(
    instance: any,
    updatePayload: any,
    type: string,
    oldProps: any,
    newProps: any,
  ) {
    Object.keys(newProps).forEach((key) => {
      // children property will be handled at appendChild() function
      if (key != "children" && oldProps[key] !== newProps[key]) {
        MachiNative.updateProps(instance.nativePtr, key, newProps[key]);
      }
    });
  },

  /* ... 나머지 필수 함수들 (createTextInstance 등) ... */
  supportsMutation: true,
  isPrimaryRenderer: true,
  supportsPersistence: false, // 우리는 Mutation(appendChild 등) 방식을 쓰므로 false
  supportsHydration: false,
};

const MachiRenderer = Reconciler(hostConfig);
export default {
  createRoot(customContainer?: Container) {
    const containerInfo: Container = customContainer ?? {
      rootTag: "DEFAULT_ROOT",
      nativePtr: (globalThis as any).nativeRootPtr,
    };

    // 2. 리액트 파이버 엔진의 실제 '루트' 생성 (createContainer)
    // 인자 설명: (containerInfo, tag, hydration, hydrationCallback, ...)
    const fiberRoot = MachiRenderer.createContainer(
      containerInfo,
      0, // ConcurrentRoot (0: Legacy, 1: Concurrent)
      null, // hydration 여부
      false, // strict mode
      null, // concurrent updates
      "", // identifierPrefix
      () => {}, // onRecoverableError
      null, // transitionCallbacks
    );
    return {
      render: (element: React.ReactNode) => {
        MachiRenderer.updateContainer(element, fiberRoot, null, () => {
          console.log("Machi: Render Success!");
        });
      },
    };
  },
};
