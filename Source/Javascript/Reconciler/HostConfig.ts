// C++에서 주입해줄 전역 객체 (MachiNative) 정의
interface MachiNative {
  createElement(type: string, props: any): number; // C++ 객체 포인터 반환
  appendChild(parentPtr: number, childPtr: number): void;
  removeChild(parentPtr: number, childPtr: number): void;
  updateProp(ptr: number, key: string, value: any): void;
}

declare const MachiNative: MachiNative;

const hostConfig: any = {
  // 1. 요소가 처음 만들어질 때 (예: <button />)
  createInstance(type: string, props: any) {
    // C++로 넘어가서 실제 객체를 만들고 그 주소를 받아옴
    const nativePtr = MachiNative.createElement(type, props);
    return {
      type,
      nativePtr, // 이 ID로 나중에 C++ 객체를 찾아 조작함
      props,
    };
  },

  // 2. 부모-자식 관계 연결
  appendChild(parent: any, child: any) {
    MachiNative.appendChild(parent.nativePtr, child.nativePtr);
  },

  appendChildToContainer(container: any, child: any) {
    MachiNative.appendChild(container.nativePtr, child.nativePtr);
  },

  // 3. 속성 변경 (텍스트 변경, 색상 변경 등)
  commitUpdate(
    instance: any,
    updatePayload: any,
    type: string,
    oldProps: any,
    newProps: any,
  ) {
    Object.keys(newProps).forEach((key) => {
      if (oldProps[key] !== newProps[key]) {
        MachiNative.updateProp(instance.nativePtr, key, newProps[key]);
      }
    });
  },

  /* ... 나머지 필수 함수들 (createTextInstance 등) ... */
  supportsMutation: true,
};

export default hostConfig;
