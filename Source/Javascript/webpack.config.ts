// webpack.config.ts 상단 수정
import webpack from "webpack";
import path from "path";
// ESM 환경에서 __dirname 구현
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
console.log(path.resolve(__dirname, "Reconciler/HostConfig.tsx"));
const config: webpack.Configuration = {
  mode: "development",
  // 엔트리 포인트를 외부 Source/javascript 폴더로 지정
  entry: path.resolve(__dirname, "Reconciler/HostConfig.ts"),

  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "machiUI-reconciler.js",
    library: {
      name: "MachiRenderer",
      type: "assign",
    },
    globalObject: "globalThis",
  },

  resolve: {
    // 라이브러리는 BuildJS/node_modules에서 찾음
    extensions: [".ts", ".tsx", ".js"],
    alias: {
      // 소스 폴더를 @src 같은 별칭으로 관리하면 편리합니다
      "@src": path.resolve(__dirname, "."),
    },
  },

  module: {
    rules: [
      {
        test: /\.(ts|tsx)$/,
        use: "ts-loader",
        include: [path.resolve(__dirname, ".")],
        exclude: /node_modules/,
      },
    ],
  },

  experiments: {
    outputModule: true,
  },
};

export default config;
