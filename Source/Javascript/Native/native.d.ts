// Native/native.d.ts

// 이 파일이 모듈임을 인식시키기 위해 빈 export를 추가합니다.
export {};

declare global {
  // 1. C++ 엔진이 주입해주는 전역 객체 인터페이스
  interface MachiNative {
    createElement(type: string, props: any): number;
    createText(text: string): number;
    appendChild(parentPtr: number, childPtr: number): void;
    updateProps(ptr: number, key: string, value: any): void;
    // 필요한 경우 removeChild 등도 추가
    removeChild(parentPtr: number, childPtr: number): void;
  }

  // 2. 실제 전역 변수 선언
  var MachiNative: MachiNative;

  // 3. 리컨실러에서 공통으로 사용할 타입들
  // export 없이 declare global 안에 두면 어디서든 'Instance'로 접근 가능합니다.
  type Instance = {
    type: string;
    props: any;
    nativePtr: number;
  };

  type TextInstance = {
    text: string;
    nativePtr: number;
  };

  type Container = {
    rootTag: string;
    nativePtr: number;
  };

  type Props = any;
}
