import React from "react";
import type { ReactNode } from "react";

export type NativeViewProps = Omit<Props, "type"> & {
  type: string;
  viewId?: string;
  children?: ReactNode;
};

function parsePayload(payload: unknown) {
  if (payload == null || payload === "") {
    return null;
  }
  if (typeof payload !== "string") {
    return payload;
  }
  try {
    return JSON.parse(payload);
  } catch {
    return payload;
  }
}

export function NativeView({ type, children, ...props }: NativeViewProps) {
  return React.createElement(
    "native-view",
    {
      ...props,
      nativeViewType: type,
    },
    children,
  );
}

export const actions = {
  invokeSync(name: string, payload?: unknown) {
    if (typeof Machi !== "undefined" && Machi.actions?.invokeSync) {
      return Machi.actions.invokeSync(name, payload);
    }

    const result = MachiNative.invokeAction(name, JSON.stringify(payload ?? null));
    if (!result.ok) {
      throw new Error(result.error || `Action failed: ${name}`);
    }
    return parsePayload(result.payload);
  },

  invoke(name: string, payload?: unknown) {
    if (typeof Machi !== "undefined" && Machi.actions?.invoke) {
      return Machi.actions.invoke(name, payload);
    }

    return new Promise((resolve, reject) => {
      try {
        resolve(actions.invokeSync(name, payload));
      } catch (error) {
        reject(error);
      }
    });
  },
};

export const MachiHost = {
  actions,
  NativeView,
};
