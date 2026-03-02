// assets/App.ts
import MachiRenderer from "@machi/core/Reconciler/HostConfig"



const root = MachiRenderer.createRoot();

const App = () => {
  // 리액트 컴포넌트 로직
  return (<div>Hello, Machi!</div>);
};



root.render(<App />);

