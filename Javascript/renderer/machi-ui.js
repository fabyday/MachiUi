// JavaScript/renderer/machi-ui.js
import Reconciler from 'react-reconciler';

const HostConfig = {
  createInstance(type, props) {
    // 사용자가 <div>를 쓰면 type은 "div"로 들어옵니다.
    // 이를 C++ 전역 함수인 __Machi__.createNode로 전달합니다.
    return globalThis.__Machi__.createNode(type, props.style || {});
  },
  appendChildToContainer(container, child) {
    globalThis.__Machi__.appendChild(container, child);
  },
  appendInitialChild(parent, child) {
    globalThis.__Machi__.appendChild(parent, child);
  },
  // 테스트를 위해 필요한 최소한의 빈 함수들
  supportsMutation: true,
  prepareUpdate: () => null,
  commitUpdate: () => {},
  getChildHostContext: () => ({}),
  getRootHostContext: () => ({}),
  shouldSetTextContent: () => false,
  createTextInstance: () => {},
  finalizeInitialChildren: () => false,
  clearContainer: () => {},
};

const reconciler = Reconciler(HostConfig);

export const Machi = {
  render(element, cppRootHandle) {
    const container = reconciler.createContainer(cppRootHandle, 0, false, null);
    reconciler.updateContainer(element, container, null, null);
  }
};