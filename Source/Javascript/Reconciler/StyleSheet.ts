type StyleMap = Record<string, unknown>;

const classStyles = new Map<string, StyleMap>();
const spacingUnit = 4;

const colorPalette: Record<string, string> = {
  black: "#000000",
  white: "#ffffff",
  "slate-50": "#f8fafc",
  "slate-100": "#f1f5f9",
  "slate-200": "#e2e8f0",
  "slate-300": "#cbd5e1",
  "slate-500": "#64748b",
  "slate-700": "#334155",
  "slate-800": "#1e293b",
  "slate-900": "#0f172a",
  "emerald-400": "#34d399",
  "emerald-500": "#10b981",
  "emerald-600": "#059669",
  "blue-500": "#3b82f6",
  "blue-600": "#2563eb",
  "amber-400": "#fbbf24",
  "amber-500": "#f59e0b",
  "rose-500": "#f43f5e",
  "rose-600": "#e11d48",
};

const textSizes: Record<string, number> = {
  xs: 12,
  sm: 14,
  base: 16,
  lg: 18,
  xl: 20,
  "2xl": 24,
  "3xl": 30,
};

const staticUtilityStyles: Record<string, StyleMap> = {
  flex: { display: "flex" },
  hidden: { display: "none" },
  "flex-row": { flexDirection: "row" },
  "flex-col": { flexDirection: "column" },
  "flex-wrap": { flexWrap: "wrap" },
  "items-start": { alignItems: "flex-start" },
  "items-center": { alignItems: "center" },
  "items-end": { alignItems: "flex-end" },
  "items-stretch": { alignItems: "stretch" },
  "justify-start": { justifyContent: "flex-start" },
  "justify-center": { justifyContent: "center" },
  "justify-end": { justifyContent: "flex-end" },
  "justify-between": { justifyContent: "space-between" },
  "flex-1": { flex: 1 },
  grow: { flexGrow: 1 },
  "shrink-0": { flexShrink: 0 },
  "w-full": { width: "100%" },
  "h-full": { height: "100%" },
  "w-screen": { width: "100%" },
  "h-screen": { height: "100%" },
  "overflow-hidden": { overflow: "hidden" },
  "font-normal": { fontWeight: 400 },
  "font-medium": { fontWeight: 500 },
  "font-semibold": { fontWeight: 600 },
  "font-bold": { fontWeight: 700 },
  "text-left": { textAlign: "left" },
  "text-center": { textAlign: "center" },
  "text-right": { textAlign: "right" },
  "animate-pulse": { animation: "pulse 1.6s ease-in-out infinite" },
};

function toCamelCase(property: string) {
  return property.trim().replace(/-([a-z])/g, (_, character: string) => character.toUpperCase());
}

function parseDeclarations(body: string) {
  const style: StyleMap = {};

  for (const declaration of body.split(";")) {
    const separator = declaration.indexOf(":");
    if (separator < 0) {
      continue;
    }

    const key = toCamelCase(declaration.slice(0, separator));
    const value = declaration.slice(separator + 1).trim();
    if (key.length === 0 || value.length === 0) {
      continue;
    }

    style[key] = value;
  }

  return style;
}

function parseBracketValue(token: string, prefix: string) {
  const open = `${prefix}-[`;
  if (!token.startsWith(open) || !token.endsWith("]")) {
    return null;
  }

  return token.slice(open.length, -1);
}

function spacingValue(rawValue: string) {
  const parsed = Number(rawValue);
  if (!Number.isFinite(parsed)) {
    return null;
  }
  return parsed * spacingUnit;
}

function resolveSpacingUtility(token: string): StyleMap | null {
  const match = token.match(/^(p|px|py|pt|pr|pb|pl|m|mx|my|mt|mr|mb|ml|gap)-(\d+)$/);
  if (match == null) {
    return null;
  }

  const value = spacingValue(match[2]);
  if (value == null) {
    return null;
  }

  switch (match[1]) {
    case "p":
      return { padding: value };
    case "px":
      return { paddingHorizontal: value };
    case "py":
      return { paddingVertical: value };
    case "pt":
      return { paddingTop: value };
    case "pr":
      return { paddingRight: value };
    case "pb":
      return { paddingBottom: value };
    case "pl":
      return { paddingLeft: value };
    case "m":
      return { margin: value };
    case "mx":
      return { marginHorizontal: value };
    case "my":
      return { marginVertical: value };
    case "mt":
      return { marginTop: value };
    case "mr":
      return { marginRight: value };
    case "mb":
      return { marginBottom: value };
    case "ml":
      return { marginLeft: value };
    case "gap":
      return { gap: value };
  }

  return null;
}

function resolveSizeUtility(token: string): StyleMap | null {
  const arbitraryWidth = parseBracketValue(token, "w");
  if (arbitraryWidth != null) {
    return { width: arbitraryWidth };
  }

  const arbitraryHeight = parseBracketValue(token, "h");
  if (arbitraryHeight != null) {
    return { height: arbitraryHeight };
  }

  const match = token.match(/^(w|h)-(\d+)$/);
  if (match == null) {
    return null;
  }

  const value = spacingValue(match[2]);
  if (value == null) {
    return null;
  }

  return match[1] === "w" ? { width: value } : { height: value };
}

function resolveColorUtility(token: string): StyleMap | null {
  const arbitraryBackground = parseBracketValue(token, "bg");
  if (arbitraryBackground != null) {
    return { backgroundColor: arbitraryBackground };
  }

  const arbitraryText = parseBracketValue(token, "text");
  if (arbitraryText != null) {
    return { color: arbitraryText };
  }

  if (token.startsWith("bg-")) {
    const color = colorPalette[token.slice(3)];
    return color == null ? null : { backgroundColor: color };
  }

  if (token.startsWith("text-")) {
    const textToken = token.slice(5);
    if (textSizes[textToken] != null) {
      return { fontSize: textSizes[textToken] };
    }

    const color = colorPalette[textToken];
    return color == null ? null : { color };
  }

  return null;
}

function resolveLeadingUtility(token: string): StyleMap | null {
  const match = token.match(/^leading-(\d+)$/);
  if (match == null) {
    return null;
  }

  const value = spacingValue(match[1]);
  return value == null ? null : { lineHeight: value };
}

function resolveUtilityClass(token: string): StyleMap {
  return {
    ...(staticUtilityStyles[token] ?? {}),
    ...(resolveSpacingUtility(token) ?? {}),
    ...(resolveSizeUtility(token) ?? {}),
    ...(resolveColorUtility(token) ?? {}),
    ...(resolveLeadingUtility(token) ?? {}),
  };
}

export function registerStylesheet(cssText: string) {
  const withoutComments = cssText.replace(/\/\*[\s\S]*?\*\//g, "");
  const rulePattern = /\.([A-Za-z0-9_-]+)\s*\{([^}]*)\}/g;

  for (const match of withoutComments.matchAll(rulePattern)) {
    const className = match[1];
    const parsed = parseDeclarations(match[2]);
    classStyles.set(className, {
      ...(classStyles.get(className) ?? {}),
      ...parsed,
    });
  }
}

export function resolveClassName(className: unknown) {
  if (typeof className !== "string") {
    return {};
  }

  const resolved: StyleMap = {};
  for (const token of className.split(/\s+/)) {
    if (token.length === 0) {
      continue;
    }
    Object.assign(resolved, classStyles.get(token), resolveUtilityClass(token));
  }

  return resolved;
}
