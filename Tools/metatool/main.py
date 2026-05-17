from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import json
import os
import os.path as osp
import re
import sys
from typing import Any, Iterable

try:
    import yaml  # type: ignore
except ImportError:  # pragma: no cover - optional dependency
    yaml = None


CPP_EXTENSIONS = (".h", ".hh", ".hpp", ".hxx", ".cpp", ".cc", ".cxx")
DEFAULT_EXCLUDE_DIRS = {
    ".git",
    ".vs",
    ".vscode",
    "build",
    "cmake-build-debug",
    "cmake-build-release",
    "node_modules",
    "ThirdParty",
}

JS_MARKER_RE = re.compile(
    r"@(?P<tag>js|machi[-_]?js|bind|native)\b(?P<body>[^\r\n*]*)",
    re.IGNORECASE,
)


@dataclasses.dataclass
class Param:
    name: str
    cpp_type: str
    ts_type: str
    optional: bool = False


@dataclasses.dataclass
class FunctionMeta:
    name: str
    return_cpp_type: str
    return_ts_type: str
    params: list[Param]
    qualified_name: str = ""
    action_name: str = ""
    source: str = ""
    target: str = ""
    is_static: bool = False
    is_constructor: bool = False


@dataclasses.dataclass
class PropertyMeta:
    name: str
    cpp_type: str
    ts_type: str
    readonly: bool = False


@dataclasses.dataclass
class ClassMeta:
    name: str
    kind: str = "class"
    extends: str | None = None
    methods: list[FunctionMeta] = dataclasses.field(default_factory=list)
    properties: list[PropertyMeta] = dataclasses.field(default_factory=list)
    source: str = ""


@dataclasses.dataclass
class MetaModel:
    classes: list[ClassMeta] = dataclasses.field(default_factory=list)
    functions: list[FunctionMeta] = dataclasses.field(default_factory=list)

    def extend(self, other: "MetaModel") -> None:
        self.classes.extend(other.classes)
        self.functions.extend(other.functions)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate TypeScript declaration metadata from C++ files marked for JS binding."
    )
    parser.add_argument("--config", type=str, help="JSON or YAML config file path")
    parser.add_argument("--verbose", action="store_true", help="enable verbose output")
    parser.add_argument("--debug", action="store_true", help="enable debug output")
    parser.add_argument("--output", type=str, help="output .d.ts/.json file path or output directory")
    parser.add_argument("--output_dir", type=str, help="directory where generated type metadata is written")
    parser.add_argument("--output_name", type=str, help="generated output filename inside output_dir")
    parser.add_argument("--input", action="append", help="input C++ file path; can be repeated")
    parser.add_argument("--input_dir", action="append", help="input directory path; can be repeated")
    parser.add_argument(
        "--jobs",
        type=int,
        default=None,
        help="number of files to parse concurrently. 0 means auto, 1 means sequential",
    )
    parser.add_argument(
        "--format",
        choices=("dts", "json"),
        default=None,
        help="output format. Defaults to dts unless output ends with .json",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="parse all public declarations. This is the default for explicitly selected inputs.",
    )
    parser.add_argument(
        "--marked-only",
        action="store_true",
        help="only parse declarations preceded by @js/@bind comments",
    )
    parser.add_argument(
        "--quickjs",
        action="store_true",
        help="infer declarations from QuickJS JS_NewCFunction registrations",
    )
    parser.add_argument(
        "--no-quickjs",
        action="store_true",
        help="disable QuickJS registration inference",
    )
    parser.add_argument(
        "--target",
        default=None,
        help="default TypeScript interface target for free/native functions",
    )
    return parser.parse_args()


def read_text(path: str) -> str:
    encodings = ("utf-8-sig", "utf-8", "cp949")
    last_error: UnicodeDecodeError | None = None
    for encoding in encodings:
        try:
            with open(path, "r", encoding=encoding) as f:
                return f.read()
        except UnicodeDecodeError as error:
            last_error = error
    if last_error:
        raise last_error
    raise RuntimeError(f"failed to read {path}")


def write_text(path: str, content: str) -> None:
    parent = osp.dirname(osp.abspath(path))
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)


def load_config(path: str) -> dict[str, Any]:
    text = read_text(path)
    extension = osp.splitext(path)[1].lower()

    if extension == ".json":
        data = json.loads(text)
    else:
        if yaml is None:
            raise RuntimeError(
                "YAML config requires PyYAML. Use a JSON config or install PyYAML."
            )
        data = yaml.safe_load(text)

    if data is None:
        return {}
    if not isinstance(data, dict):
        raise RuntimeError("metatool config root must be an object")
    return data


def as_list(value: Any) -> list[Any]:
    if value is None:
        return []
    if isinstance(value, list):
        return value
    if isinstance(value, tuple):
        return list(value)
    return [value]


def first_config_value(config: dict[str, Any], *keys: str) -> Any:
    for key in keys:
        if key in config and config[key] is not None:
            return config[key]
    return None


def merge_args_with_config(args: argparse.Namespace) -> dict[str, Any]:
    config: dict[str, Any] = {}
    if args.config:
        config.update(load_config(args.config))
    format_explicit = "format" in config and config.get("format") is not None

    if args.verbose:
        config["verbose"] = True
    if args.debug:
        config["debug"] = True
    if args.output:
        config["output"] = args.output
    if args.output_dir:
        config["output_dir"] = args.output_dir
    if args.output_name:
        config["output_name"] = args.output_name
    if args.input:
        config["input"] = args.input
    if args.input_dir:
        config["input_dir"] = args.input_dir
    if args.jobs is not None:
        config["jobs"] = args.jobs
    if args.format:
        config["format"] = args.format
        format_explicit = True
    if args.all:
        config["all"] = True
    if args.marked_only:
        config["marked_only"] = True
    if args.quickjs:
        config["quickjs"] = True
    if args.no_quickjs:
        config["quickjs"] = False
    if args.target:
        config["target"] = args.target

    config.setdefault("output", None)
    config.setdefault("output_dir", None)
    config.setdefault("output_name", None)
    config.setdefault("format", None)
    config.setdefault("target", "")
    config.setdefault("all", True)
    config.setdefault("marked_only", False)
    config.setdefault("quickjs", False)
    config.setdefault("jobs", 0)
    config.setdefault("extensions", list(CPP_EXTENSIONS))
    config.setdefault("exclude", list(DEFAULT_EXCLUDE_DIRS))
    config["_format_explicit"] = format_explicit

    if "input" not in config and "inputs" in config:
        config["input"] = config["inputs"]
    if "input_dir" not in config:
        input_dir = first_config_value(config, "input_dirs", "inputDirs", "source_dir", "sourceDir")
        if input_dir is not None:
            config["input_dir"] = input_dir
    if "output_dir" not in config or config["output_dir"] is None:
        output_dir = first_config_value(config, "outputDir", "out_dir", "outDir")
        if output_dir is not None:
            config["output_dir"] = output_dir
    if "output_name" not in config or config["output_name"] is None:
        output_name = first_config_value(config, "outputName", "filename", "fileName")
        if output_name is not None:
            config["output_name"] = output_name
    return config


def set_env(config: dict[str, Any]) -> None:
    env = config.get("env")
    if not env:
        return
    if not isinstance(env, dict):
        raise RuntimeError("config env must be an object")
    for key, value in env.items():
        os.environ[str(key)] = str(value)


def normalize_path(path: str) -> str:
    return osp.abspath(osp.expanduser(path))


def collect_input_files(config: dict[str, Any]) -> list[str]:
    files: list[str] = []
    extensions = tuple(str(ext) for ext in as_list(config.get("extensions")) or CPP_EXTENSIONS)
    exclude = {str(name) for name in as_list(config.get("exclude"))}

    for path_value in as_list(config.get("input")):
        path = normalize_path(str(path_value))
        if osp.isfile(path):
            files.append(path)
        else:
            raise FileNotFoundError(path)

    for dir_value in as_list(config.get("input_dir")):
        root = normalize_path(str(dir_value))
        if not osp.isdir(root):
            raise FileNotFoundError(root)

        for current, dirs, names in os.walk(root):
            dirs[:] = [name for name in dirs if name not in exclude]
            for name in names:
                if name.endswith(extensions):
                    files.append(osp.join(current, name))

    return sorted(dict.fromkeys(files))


def resolve_jobs(config: dict[str, Any], file_count: int) -> int:
    requested = int(config.get("jobs") or 0)
    if requested < 0:
        raise RuntimeError("jobs must be 0 or greater")
    if requested == 1 or file_count <= 1:
        return 1
    if requested > 1:
        return requested
    cpu_count = os.cpu_count() or 4
    return max(1, min(file_count, cpu_count))


def resolve_output_path(config: dict[str, Any], output_format: str) -> str | None:
    output_dir = config.get("output_dir")
    output_name = config.get("output_name")
    output = config.get("output")

    default_name = "metadata.json" if output_format == "json" else "types.d.ts"

    if output_dir:
        return osp.join(str(output_dir), str(output_name or default_name))

    if output:
        output_text = str(output)
        _, extension = osp.splitext(output_text)
        if not extension or osp.isdir(output_text):
            return osp.join(output_text, str(output_name or default_name))
        return output_text

    return None


def output_is_directory(config: dict[str, Any]) -> bool:
    if config.get("output_dir"):
        return True
    output = config.get("output")
    if not output:
        return False
    output_text = str(output)
    _, extension = osp.splitext(output_text)
    return not extension or osp.isdir(output_text)


def strip_comments(source: str) -> str:
    def replacer(match: re.Match[str]) -> str:
        text = match.group(0)
        return "\n" * text.count("\n")

    return re.sub(r"//[^\n]*|/\*.*?\*/", replacer, source, flags=re.DOTALL)


def strip_string_literals(source: str) -> str:
    pattern = r'R"\([^)]*\)(.*?)\)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, '""', source, flags=re.DOTALL)


def remove_comments_and_strings(source: str) -> str:
    return strip_string_literals(strip_comments(source))


def split_top_level(text: str, delimiter: str = ",") -> list[str]:
    parts: list[str] = []
    start = 0
    angle = paren = bracket = brace = 0
    for index, char in enumerate(text):
        if char == "<":
            angle += 1
        elif char == ">":
            angle = max(0, angle - 1)
        elif char == "(":
            paren += 1
        elif char == ")":
            paren = max(0, paren - 1)
        elif char == "[":
            bracket += 1
        elif char == "]":
            bracket = max(0, bracket - 1)
        elif char == "{":
            brace += 1
        elif char == "}":
            brace = max(0, brace - 1)
        elif char == delimiter and not angle and not paren and not bracket and not brace:
            parts.append(text[start:index].strip())
            start = index + 1
    parts.append(text[start:].strip())
    return [part for part in parts if part]


def remove_default_value(param: str) -> tuple[str, bool]:
    depth = 0
    for index, char in enumerate(param):
        if char in "(<{[":
            depth += 1
        elif char in ")>}]":
            depth = max(0, depth - 1)
        elif char == "=" and depth == 0:
            return param[:index].strip(), True
    return param.strip(), False


def normalize_cpp_type(cpp_type: str) -> str:
    cpp_type = cpp_type.strip()
    cpp_type = re.sub(r"\b(const|volatile|mutable|static|inline|virtual|constexpr|friend|extern)\b", "", cpp_type)
    cpp_type = re.sub(r"\b(class|struct|enum)\s+", "", cpp_type)
    cpp_type = re.sub(r"\s+", " ", cpp_type)
    return cpp_type.strip()


def strip_namespace(type_name: str) -> str:
    return re.sub(r"\bstd::", "", type_name)


def strip_outer_template(type_name: str, template_name: str) -> str | None:
    names = [template_name, f"std::{template_name}"]
    for name in names:
        prefix = f"{name}<"
        if type_name.startswith(prefix) and type_name.endswith(">"):
            return type_name[len(prefix) : -1].strip()
    return None


def map_cpp_type_to_ts(cpp_type: str) -> str:
    original = normalize_cpp_type(cpp_type)
    if not original:
        return "unknown"

    pointer_like = "*" in original
    type_name = original.replace("&", " ").replace("*", " ")
    type_name = re.sub(r"\s+", " ", type_name).strip()
    compact = type_name.replace(" ", "")
    compact_no_std = strip_namespace(compact)

    if pointer_like and compact_no_std in {"char", "wchar_t", "char8_t", "char16_t", "char32_t"}:
        return "string"
    if pointer_like:
        return "NativePtr"

    lower = compact_no_std.lower()
    if lower in {"void"}:
        return "void"
    if lower in {"bool", "boolean"}:
        return "boolean"
    if compact_no_std in {"string", "wstring", "u8string", "u16string", "u32string"}:
        return "string"
    if lower in {
        "char",
        "signedchar",
        "unsignedchar",
        "short",
        "unsignedshort",
        "int",
        "unsignedint",
        "long",
        "unsignedlong",
        "longlong",
        "unsignedlonglong",
        "float",
        "double",
        "longdouble",
        "size_t",
        "ssize_t",
        "int8_t",
        "uint8_t",
        "int16_t",
        "uint16_t",
        "int32_t",
        "uint32_t",
    }:
        return "number"
    if lower in {"int64_t", "uint64_t", "intptr_t", "uintptr_t"}:
        return "NativePtr"
    if compact_no_std in {"JSValue", "JSValueConst", "JSAtom"}:
        return "unknown"

    optional_inner = strip_outer_template(type_name, "optional")
    if optional_inner:
        return f"{map_cpp_type_to_ts(optional_inner)} | undefined"

    vector_inner = strip_outer_template(type_name, "vector")
    if vector_inner:
        return f"{map_cpp_type_to_ts(vector_inner)}[]"

    array_inner = strip_outer_template(type_name, "array")
    if array_inner:
        first = split_top_level(array_inner)[0]
        return f"{map_cpp_type_to_ts(first)}[]"

    variant_inner = strip_outer_template(type_name, "variant")
    if variant_inner:
        mapped = [map_cpp_type_to_ts(part) for part in split_top_level(variant_inner)]
        mapped = [part for part in mapped if part != "void"]
        return " | ".join(dict.fromkeys(mapped)) or "unknown"

    map_inner = strip_outer_template(type_name, "unordered_map") or strip_outer_template(type_name, "map")
    if map_inner:
        parts = split_top_level(map_inner)
        if len(parts) >= 2:
            return f"Record<{map_cpp_type_to_ts(parts[0])}, {map_cpp_type_to_ts(parts[1])}>"
        return "Record<string, unknown>"

    function_inner = strip_outer_template(type_name, "function")
    if function_inner:
        match = re.match(r"(?P<ret>.+?)\((?P<params>.*)\)$", function_inner.strip())
        if match:
            params = parse_params(match.group("params"))
            param_text = ", ".join(param_to_ts(param, index) for index, param in enumerate(params))
            return f"({param_text}) => {map_cpp_type_to_ts(match.group('ret'))}"
        return "(...args: unknown[]) => unknown"

    if "::" in compact:
        compact = compact.split("::")[-1]
    if compact in {"monostate", "nullptr_t"}:
        return "null"
    if compact.endswith("AttrValue"):
        return "unknown"
    return compact or "unknown"


def parse_params(params_text: str) -> list[Param]:
    params_text = params_text.strip()
    if not params_text or params_text == "void":
        return []

    params: list[Param] = []
    for index, raw_param in enumerate(split_top_level(params_text)):
        raw_param, optional = remove_default_value(raw_param)
        raw_param = re.sub(r"\b(final|override)\b", "", raw_param).strip()
        if not raw_param or raw_param == "void":
            continue

        raw_param = raw_param.replace("&&", " && ").replace("&", " & ").replace("*", " * ")
        raw_param = re.sub(r"\s+", " ", raw_param).strip()
        array_suffix = ""
        array_match = re.search(r"(\[[^\]]*\])+$", raw_param)
        if array_match:
            array_suffix = array_match.group(0)
            raw_param = raw_param[: array_match.start()].strip()

        match = re.match(r"(?P<type>.+?)\s+(?P<name>[A-Za-z_]\w*)$", raw_param)
        if match:
            cpp_type = f"{match.group('type').strip()} {array_suffix}".strip()
            name = match.group("name")
        else:
            cpp_type = raw_param
            name = f"arg{index}"

        if name in {"const", "volatile"}:
            name = f"arg{index}"

        params.append(
            Param(
                name=name,
                cpp_type=normalize_cpp_type(cpp_type),
                ts_type=map_cpp_type_to_ts(cpp_type),
                optional=optional,
            )
        )
    return params


def parse_marker(text: str) -> dict[str, Any]:
    match = JS_MARKER_RE.search(text)
    if not match:
        return {}

    body = match.group("body").strip()
    meta: dict[str, Any] = {"tag": match.group("tag")}
    for token in re.findall(r'(\w+)\s*[:=]\s*("[^"]*"|\'[^\']*\'|[^\s,]+)|(\w+)', body):
        key, value, bare = token
        if bare:
            meta[bare.lower()] = True
            continue
        value = value.strip().strip("\"'")
        key = key.lower()
        meta[key] = value
    return meta


def find_next_declaration(lines: list[str], start_index: int) -> tuple[int, str]:
    index = start_index
    while index < len(lines):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("#"):
            index += 1
            continue
        break

    declaration_lines: list[str] = []
    brace_depth = 0
    seen_body = False
    while index < len(lines):
        line = lines[index]
        declaration_lines.append(line)
        brace_depth += line.count("{") - line.count("}")
        if "{" in line:
            seen_body = True
        stripped = line.strip()
        if stripped.endswith(";") and brace_depth <= 0:
            break
        if seen_body and brace_depth <= 0:
            if stripped.endswith(";"):
                break
            if re.search(r"}\s*;?\s*$", stripped):
                if ";" in stripped or stripped == "}":
                    break
        index += 1

    return index, "\n".join(declaration_lines)


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    for index in range(open_index, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index
    return -1


def parse_function_signature(signature: str, source: str = "", target: str = "") -> FunctionMeta | None:
    signature = strip_comments(signature)
    signature = re.sub(r"\s+", " ", signature).strip()
    signature = signature.rstrip(";")
    signature = re.sub(r"\s*(?:noexcept|override|final)\b", "", signature).strip()
    signature = re.sub(r"\s*=\s*0\s*$", "", signature).strip()

    signature = re.sub(
        r"^(?:template\s*<[^>]+>\s*)?(?:MACHI_UI_STATIC\s+)?",
        "",
        signature,
    ).strip()

    match = re.match(
        r"(?P<prefix>(?:static|inline|virtual|constexpr|extern|friend|explicit)\s+)*"
        r"(?P<ret>[A-Za-z_~][\w:<>,\s\*&]+?)\s+"
        r"(?P<name>(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*"
        r"\((?P<params>.*)\)\s*(?P<const>const)?$",
        signature,
    )
    if not match:
        return None

    full_name = match.group("name")
    name = full_name.split("::")[-1]
    ret = normalize_cpp_type(match.group("ret"))
    return FunctionMeta(
        name=name,
        return_cpp_type=ret,
        return_ts_type=map_cpp_type_to_ts(ret),
        params=parse_params(match.group("params")),
        qualified_name=full_name.replace("::", "."),
        action_name=full_name.replace("::", "."),
        source=source,
        target=target,
        is_static="static" in (match.group("prefix") or ""),
    )


def iter_access_segments(body: str, default_access: str) -> Iterable[tuple[str, str]]:
    access_pattern = re.compile(r"^\s*(public|private|protected)\s*:\s*$", re.MULTILINE)
    current_access = default_access
    last = 0
    for match in access_pattern.finditer(body):
        yield current_access, body[last : match.start()]
        current_access = match.group(1)
        last = match.end()
    yield current_access, body[last:]


def remove_function_bodies(segment: str) -> str:
    result: list[str] = []
    index = 0
    while index < len(segment):
        open_index = segment.find("{", index)
        if open_index < 0:
            result.append(segment[index:])
            break
        result.append(segment[index:open_index])
        close_index = find_matching_brace(segment, open_index)
        if close_index < 0:
            break
        result.append(";")
        index = close_index + 1
    return "".join(result)


def parse_class_declaration(declaration: str, marker: dict[str, Any], source: str = "") -> ClassMeta | None:
    clean = strip_comments(declaration)
    match = re.search(
        r"\b(?P<kind>class|struct)\s+(?P<name>[A-Za-z_]\w*)\s*(?P<bases>[^{};]*)\{",
        clean,
        flags=re.DOTALL,
    )
    if not match:
        return None

    open_index = clean.find("{", match.start())
    close_index = find_matching_brace(clean, open_index)
    if close_index < 0:
        return None

    kind = match.group("kind")
    cpp_name = match.group("name")
    name = str(marker.get("name") or marker.get("as") or cpp_name)
    bases = match.group("bases") or ""
    extends: str | None = None
    base_match = re.search(r":\s*(?:public|protected|private)?\s*([A-Za-z_]\w*)", bases)
    if base_match:
        extends = base_match.group(1)

    body = clean[open_index + 1 : close_index]
    class_meta = ClassMeta(name=name, kind=kind, extends=extends, source=source)
    default_access = "public" if kind == "struct" else "private"

    for access, segment in iter_access_segments(body, default_access):
        if access != "public":
            continue

        method_segment = segment
        for func_match in re.finditer(
            r"(?P<signature>"
            r"(?:static|inline|virtual|constexpr|explicit|MACHI_UI_STATIC|\s)+"
            r"[A-Za-z_~][\w:<>,\s\*&]*\s+"
            r"[A-Za-z_]\w*\s*\([^;{}()]*\)\s*(?:const)?\s*(?:override|final)?\s*)"
            r"(?P<end>;|\{)",
            method_segment,
            flags=re.MULTILINE,
        ):
            signature = func_match.group("signature")
            function = parse_function_signature(signature, source=source)
            if function is None:
                continue
            if function.name == cpp_name or function.name.startswith("~"):
                function.is_constructor = function.name == cpp_name
                function.return_ts_type = ""
            else:
                function.qualified_name = f"{name}.{function.name}"
                function.action_name = function.qualified_name
            class_meta.methods.append(function)

        constructor_pattern = re.compile(
            rf"\b(?P<name>{re.escape(cpp_name)})\s*\((?P<params>[^;{{}}()]*)\)\s*(?P<end>;|\{{)",
            flags=re.MULTILINE,
        )
        for ctor_match in constructor_pattern.finditer(method_segment):
            class_meta.methods.append(
                FunctionMeta(
                    name=name,
                    return_cpp_type="",
                    return_ts_type="",
                    params=parse_params(ctor_match.group("params")),
                    qualified_name=name,
                    action_name=name,
                    source=source,
                    is_constructor=True,
                )
            )

        property_segment = remove_function_bodies(method_segment)
        for statement in split_top_level(property_segment.replace("\n", " "), ";"):
            if "(" in statement or ")" in statement:
                continue
            statement = statement.strip()
            if not statement or statement in {"public:", "private:", "protected:"}:
                continue
            if re.search(r"\b(using|typedef|friend|return|if|for|while|switch)\b", statement):
                continue
            prop_match = re.match(
                r"(?P<type>[A-Za-z_][\w:<>,\s\*&]+?)\s+(?P<name>[A-Za-z_]\w*)\s*(?:=.*)?$",
                statement,
            )
            if not prop_match:
                continue
            cpp_type = normalize_cpp_type(prop_match.group("type"))
            prop_name = prop_match.group("name")
            class_meta.properties.append(
                PropertyMeta(
                    name=prop_name,
                    cpp_type=cpp_type,
                    ts_type=map_cpp_type_to_ts(cpp_type),
                    readonly=False,
                )
            )

    return class_meta


def blank_ranges(text: str, ranges: Iterable[tuple[int, int]]) -> str:
    chars = list(text)
    for start, end in ranges:
        for index in range(max(0, start), min(len(chars), end)):
            if chars[index] != "\n":
                chars[index] = " "
    return "".join(chars)


def find_class_ranges(source: str) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    for class_match in re.finditer(r"\b(?:class|struct)\s+[A-Za-z_]\w*[^{};]*\{", source):
        open_index = source.find("{", class_match.start())
        close_index = find_matching_brace(source, open_index)
        if close_index < 0:
            continue
        end = close_index + 1
        while end < len(source) and source[end].isspace():
            end += 1
        if end < len(source) and source[end] == ";":
            end += 1
        ranges.append((class_match.start(), end))
    return ranges


def find_namespace_ranges(source: str) -> list[tuple[int, int, str]]:
    ranges: list[tuple[int, int, str]] = []
    pattern = re.compile(r"\bnamespace\s+(?P<name>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{")
    for match in pattern.finditer(source):
        open_index = source.find("{", match.start())
        close_index = find_matching_brace(source, open_index)
        if close_index < 0:
            continue
        ranges.append((match.start(), close_index + 1, match.group("name")))
    return ranges


def namespace_prefix_at(ranges: list[tuple[int, int, str]], index: int) -> str:
    names: list[str] = []
    for start, end, name in sorted(ranges, key=lambda item: item[0]):
        if start <= index < end:
            names.extend(part for part in name.split("::") if part)
    return ".".join(names)


def top_level_namespace_ranges(ranges: list[tuple[int, int, str]]) -> list[tuple[int, int, str]]:
    top_level: list[tuple[int, int, str]] = []
    for namespace_range in sorted(ranges, key=lambda item: (item[0], -(item[1] - item[0]))):
        start, end, _ = namespace_range
        if any(parent_start < start and end <= parent_end for parent_start, parent_end, _ in top_level):
            continue
        top_level.append(namespace_range)
    return top_level


def apply_action_name(function: FunctionMeta, prefix: str = "") -> FunctionMeta:
    qualified = function.qualified_name or function.name
    if prefix and "." not in qualified:
        qualified = f"{prefix}.{qualified}"
    function.action_name = function.action_name or qualified.replace("::", ".")
    function.qualified_name = qualified
    return function


def prefix_action_model(model: MetaModel, prefix: str) -> MetaModel:
    prefix = prefix.strip(".")
    if not prefix:
        return model
    for function in model.functions:
        action_name = function.action_name or function.qualified_name or function.name
        if not action_name.startswith(f"{prefix}."):
            function.action_name = f"{prefix}.{action_name}"
            function.qualified_name = f"{prefix}.{function.qualified_name or function.name}"
    for cls in model.classes:
        for method in cls.methods:
            action_name = method.action_name or method.qualified_name or f"{cls.name}.{method.name}"
            if not action_name.startswith(f"{prefix}."):
                method.action_name = f"{prefix}.{action_name}"
                method.qualified_name = f"{prefix}.{method.qualified_name or action_name}"
    return model


def parse_all_declarations(source: str, source_path: str = "", default_target: str = "") -> MetaModel:
    model = MetaModel()
    clean = strip_comments(source)
    namespace_ranges = find_namespace_ranges(clean)
    top_namespaces = top_level_namespace_ranges(namespace_ranges)

    for start, end, name in top_namespaces:
        open_index = clean.find("{", start)
        if open_index < 0:
            continue
        namespace_body = clean[open_index + 1 : end - 1]
        namespace_prefix = name.replace("::", ".")
        model.extend(prefix_action_model(parse_all_declarations(namespace_body, source_path, default_target), namespace_prefix))

    clean_without_namespaces = blank_ranges(clean, [(start, end) for start, end, _ in top_namespaces])
    class_ranges = find_class_ranges(clean_without_namespaces)

    for start, end in class_ranges:
        class_meta = parse_class_declaration(clean_without_namespaces[start:end], {"tag": "all"}, source_path)
        if class_meta:
            model.classes.append(class_meta)

    free_source = blank_ranges(clean_without_namespaces, class_ranges)
    free_source = remove_function_bodies(free_source)
    statement_pattern = re.compile(
        r"(?P<signature>"
        r"(?:template\s*<[^;{}]+>\s*)?"
        r"(?:MACHI_UI_STATIC\s+)?"
        r"(?:static|inline|constexpr|extern)?\s*"
        r"[A-Za-z_~][\w:<>,\s\*&]*\s+"
        r"(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*\s*"
        r"\([^;{}]*\)\s*(?:const)?\s*)"
        r";",
        flags=re.MULTILINE,
    )
    for match in statement_pattern.finditer(free_source):
        signature = match.group("signature")
        if re.search(r"\b(if|for|while|switch|catch)\s*\(", signature):
            continue
        if re.search(r"\b[A-Za-z_]\w*::[A-Za-z_]\w*\s*\(", signature):
            continue
        function = parse_function_signature(signature, source_path, default_target)
        if function:
            model.functions.append(apply_action_name(function))

    definition_pattern = re.compile(
        r"(?P<signature>"
        r"(?:template\s*<[^;{}]+>\s*)?"
        r"(?:MACHI_UI_STATIC\s+)?"
        r"(?:static|inline|constexpr|extern)?\s*"
        r"[A-Za-z_~][\w:<>,\s\*&]*\s+"
        r"(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*\s*"
        r"\([^;{}]*\)\s*(?:const)?\s*)"
        r"\{",
        flags=re.MULTILINE,
    )
    for match in definition_pattern.finditer(free_source):
        signature = match.group("signature")
        if re.search(r"\b(if|for|while|switch|catch)\s*\(", signature):
            continue
        if re.search(r"\b[A-Za-z_]\w*::[A-Za-z_]\w*\s*\(", signature):
            continue
        function = parse_function_signature(signature, source_path, default_target)
        if function:
            model.functions.append(apply_action_name(function))

    return model


def parse_annotated_declarations(
    source: str,
    source_path: str = "",
    parse_all: bool = False,
    default_target: str = "",
) -> MetaModel:
    model = MetaModel()
    lines = source.splitlines()

    if parse_all:
        return parse_all_declarations(source, source_path, default_target)

    index = 0
    while index < len(lines):
        line = lines[index]
        marker = parse_marker(line)
        if not marker:
            index += 1
            continue

        end_index, declaration = find_next_declaration(lines, index + 1)
        declaration_kind = "class" if re.search(r"\b(class|struct)\b", declaration) else "function"
        if marker.get("function"):
            declaration_kind = "function"
        elif marker.get("class") or marker.get("struct"):
            declaration_kind = "class"

        if declaration_kind == "class":
            class_meta = parse_class_declaration(declaration, marker, source_path)
            if class_meta:
                model.classes.append(class_meta)
        else:
            target = str(marker.get("target") or default_target)
            function = parse_function_signature(declaration.split("{", 1)[0], source_path, target)
            if function:
                function.name = str(marker.get("name") or marker.get("as") or function.name)
                if "return" in marker:
                    function.return_ts_type = str(marker["return"])
                elif "returns" in marker:
                    function.return_ts_type = str(marker["returns"])
                model.functions.append(function)

        index = max(end_index + 1, index + 1)

    return model


def parse_quickjs_registrations(source: str, source_path: str = "", default_target: str = "") -> MetaModel:
    model = MetaModel()
    clean = strip_comments(source)
    object_targets: dict[str, str] = {}

    for match in re.finditer(
        r"JS_SetPropertyStr\s*\(\s*[^,]+,\s*(?P<parent>[A-Za-z_]\w*)\s*,\s*"
        r'"(?P<name>[^"]+)"\s*,\s*(?P<object>[A-Za-z_]\w*)\s*\)',
        clean,
    ):
        object_targets[match.group("object")] = match.group("name")

    native_function_pattern = re.compile(
        r"JS_SetPropertyStr\s*\(\s*[^,]+,\s*(?P<object>[A-Za-z_]\w*)\s*,\s*"
        r'"(?P<prop>[^"]+)"\s*,\s*JS_NewCFunction\s*\(\s*[^,]+,\s*'
        r"(?P<cfunc>[A-Za-z_]\w*)\s*,\s*\"(?P<jsname>[^\"]+)\"\s*,\s*(?P<argc>\d+)\s*\)",
        flags=re.DOTALL,
    )
    for match in native_function_pattern.finditer(clean):
        obj = match.group("object")
        target = object_targets.get(obj, default_target if obj in {"native", "global"} else obj)
        argc = int(match.group("argc"))
        params = [
            Param(name=f"arg{index}", cpp_type="JSValue", ts_type="unknown")
            for index in range(argc)
        ]
        action_name = f"{target}.{match.group('prop')}" if target else match.group("prop")
        model.functions.append(
            FunctionMeta(
                name=match.group("prop"),
                return_cpp_type="JSValue",
                return_ts_type="unknown",
                params=params,
                qualified_name=action_name,
                action_name=action_name,
                source=source_path,
                target=target,
            )
        )

    binder_arrays: dict[str, list[FunctionMeta]] = {}
    array_pattern = re.compile(
        r"static\s+const\s+JSCFunctionListEntry\s+(?P<array>[A-Za-z_]\w*)\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        flags=re.DOTALL,
    )
    macro_pattern = re.compile(
        r"(?:MACHI_JS_CFUNC_DEF|JS_CFUNC_DEF)\s*\(\s*\"(?P<name>[^\"]+)\"\s*,\s*(?P<argc>\d+)\s*,\s*(?P<cfunc>[A-Za-z_]\w*)\s*\)"
    )
    for array_match in array_pattern.finditer(clean):
        functions: list[FunctionMeta] = []
        for macro_match in macro_pattern.finditer(array_match.group("body")):
            argc = int(macro_match.group("argc"))
            params = [
                Param(name=f"arg{index}", cpp_type="JSValue", ts_type="unknown")
                for index in range(argc)
            ]
            functions.append(
                FunctionMeta(
                    name=macro_match.group("name"),
                    return_cpp_type="JSValue",
                    return_ts_type="unknown",
                    params=params,
                    qualified_name=macro_match.group("name"),
                    action_name=macro_match.group("name"),
                    source=source_path,
                )
            )
        if functions:
            binder_arrays[array_match.group("array")] = functions

    binder_pattern = re.compile(
        r"GenericBinder\s*\(\s*\{\s*\"(?P<class>[^\"]+)\"\s*,\s*(?P<array>[A-Za-z_]\w*)\s*,"
        r".*?(?P<parent>nullptr|\"[^\"]+\")\s*\}\s*\)\s*\.Bind\s*\(",
        flags=re.DOTALL,
    )
    for binder_match in binder_pattern.finditer(clean):
        array_name = binder_match.group("array")
        if array_name not in binder_arrays:
            continue
        parent = binder_match.group("parent")
        class_meta = ClassMeta(
            name=binder_match.group("class"),
            extends=None if parent == "nullptr" else parent.strip('"'),
            methods=list(binder_arrays[array_name]),
            source=source_path,
        )
        model.classes.append(class_meta)

    return model


def parse_input_file(
    input_string: str,
    source_path: str = "",
    parse_all: bool = False,
    quickjs: bool = True,
    default_target: str = "",
) -> MetaModel:
    model = parse_annotated_declarations(
        input_string,
        source_path,
        parse_all=parse_all,
        default_target=default_target,
    )
    if quickjs:
        model.extend(parse_quickjs_registrations(input_string, source_path, default_target))
    return model


def dedupe_model(model: MetaModel) -> MetaModel:
    class_by_name: dict[str, ClassMeta] = {}
    for cls in model.classes:
        existing = class_by_name.get(cls.name)
        if existing is None:
            class_by_name[cls.name] = cls
            continue
        seen_methods = {(method.name, len(method.params)) for method in existing.methods}
        for method in cls.methods:
            key = (method.name, len(method.params))
            if key not in seen_methods:
                existing.methods.append(method)
                seen_methods.add(key)
        seen_props = {prop.name for prop in existing.properties}
        for prop in cls.properties:
            if prop.name not in seen_props:
                existing.properties.append(prop)
                seen_props.add(prop.name)

    functions_by_key: dict[tuple[str, str, int], FunctionMeta] = {}
    for function in model.functions:
        key_name = function.action_name or function.qualified_name or function.name
        functions_by_key.setdefault((function.target, key_name, len(function.params)), function)

    return MetaModel(classes=list(class_by_name.values()), functions=list(functions_by_key.values()))


def ts_identifier(name: str) -> str:
    if re.match(r"^[A-Za-z_$][\w$]*$", name):
        return name
    return json.dumps(name)


def param_to_ts(param: Param, index: int) -> str:
    name = param.name or f"arg{index}"
    optional = "?" if param.optional else ""
    return f"{ts_identifier(name)}{optional}: {param.ts_type}"


def function_to_ts(function: FunctionMeta, indent: str = "    ") -> str:
    params = ", ".join(param_to_ts(param, index) for index, param in enumerate(function.params))
    if function.is_constructor:
        return f"{indent}constructor({params});"
    prefix = "static " if function.is_static else ""
    return f"{indent}{prefix}{ts_identifier(function.name)}({params}): {function.return_ts_type};"


def free_function_to_ts(function: FunctionMeta, indent: str = "  ") -> str:
    params = ", ".join(param_to_ts(param, index) for index, param in enumerate(function.params))
    return f"{indent}function {ts_identifier(function.name)}({params}): {function.return_ts_type};"


def property_to_ts(property_meta: PropertyMeta, indent: str = "    ") -> str:
    readonly = "readonly " if property_meta.readonly else ""
    return f"{indent}{readonly}{ts_identifier(property_meta.name)}: {property_meta.ts_type};"


def model_uses_native_ptr(model: MetaModel) -> bool:
    def has_native_ptr_type(type_text: str) -> bool:
        return "NativePtr" in type_text

    for function in model.functions:
        if has_native_ptr_type(function.return_ts_type):
            return True
        if any(has_native_ptr_type(param.ts_type) for param in function.params):
            return True
    for cls in model.classes:
        for method in cls.methods:
            if has_native_ptr_type(method.return_ts_type):
                return True
            if any(has_native_ptr_type(param.ts_type) for param in method.params):
                return True
        if any(has_native_ptr_type(prop.ts_type) for prop in cls.properties):
            return True
    return False


def render_dts(model: MetaModel) -> str:
    model = dedupe_model(model)
    grouped_functions: dict[str, list[FunctionMeta]] = {}
    free_functions: list[FunctionMeta] = []
    for function in model.functions:
        if function.target:
            grouped_functions.setdefault(function.target, []).append(function)
        else:
            free_functions.append(function)

    lines: list[str] = [
        "// Generated by tools/metatool. Do not edit by hand.",
        "export {};",
        "",
        "declare global {",
    ]

    if model_uses_native_ptr(model):
        lines.append("  type NativePtr = number | bigint;")
        lines.append("")

    for function in sorted(free_functions, key=lambda item: item.name):
        lines.append(free_function_to_ts(function, "  "))
    if free_functions:
        lines.append("")

    for target in sorted(grouped_functions):
        lines.append(f"  interface {target} {{")
        for function in sorted(grouped_functions[target], key=lambda item: item.name):
            lines.append(function_to_ts(function, "    "))
        lines.append("  }")
        lines.append("")
        if target[0].isupper():
            lines.append(f"  var {target}: {target};")
            lines.append("")

    for cls in sorted(model.classes, key=lambda item: item.name):
        extends = f" extends {cls.extends}" if cls.extends else ""
        lines.append(f"  class {cls.name}{extends} {{")
        for prop in sorted(cls.properties, key=lambda item: item.name):
            lines.append(property_to_ts(prop, "    "))
        if cls.properties and cls.methods:
            lines.append("")
        for method in sorted(cls.methods, key=lambda item: (not item.is_constructor, item.name)):
            lines.append(function_to_ts(method, "    "))
        lines.append("  }")
        lines.append("")

    if lines[-1] == "":
        lines.pop()
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def dataclass_to_dict(value: Any) -> Any:
    if dataclasses.is_dataclass(value):
        return {field.name: dataclass_to_dict(getattr(value, field.name)) for field in dataclasses.fields(value)}
    if isinstance(value, list):
        return [dataclass_to_dict(item) for item in value]
    if isinstance(value, dict):
        return {key: dataclass_to_dict(item) for key, item in value.items()}
    return value


def render_json(model: MetaModel) -> str:
    return json.dumps(dataclass_to_dict(dedupe_model(model)), ensure_ascii=False, indent=2) + "\n"


def model_uses_native_ptr_in_actions(actions: list[FunctionMeta]) -> bool:
    for action in actions:
        if "NativePtr" in action.return_ts_type:
            return True
        if any("NativePtr" in param.ts_type for param in action.params):
            return True
    return False


def iter_action_functions(model: MetaModel) -> list[FunctionMeta]:
    actions: list[FunctionMeta] = []
    deduped = dedupe_model(model)
    for function in deduped.functions:
        if function.is_constructor:
            continue
        actions.append(apply_action_name(function))
    for cls in deduped.classes:
        for method in cls.methods:
            if method.is_constructor:
                continue
            if not method.action_name:
                method.action_name = f"{cls.name}.{method.name}"
            if not method.qualified_name:
                method.qualified_name = method.action_name
            actions.append(method)
    return sorted(actions, key=lambda item: item.action_name or item.name)


def render_action_dts(model: MetaModel) -> str:
    actions = iter_action_functions(model)
    lines: list[str] = [
        "// Generated by tools/metatool. Do not edit by hand.",
        "export {};",
        "",
        "declare global {",
    ]
    if model_uses_native_ptr_in_actions(actions):
        lines.append("  type NativePtr = number | bigint;")
        lines.append("")

    lines.append("  interface MetaToolActionMap {")
    for action in actions:
        params = ", ".join(param_to_ts(param, index) for index, param in enumerate(action.params))
        lines.append(f"    {json.dumps(action.action_name)}: ({params}) => {action.return_ts_type};")
    lines.append("  }")
    lines.append("")
    lines.append("  function invoke<Name extends keyof MetaToolActionMap>(")
    lines.append("    name: Name,")
    lines.append("    ...args: Parameters<MetaToolActionMap[Name]>")
    lines.append("  ): ReturnType<MetaToolActionMap[Name]>;")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def render_action_metadata(model: MetaModel) -> str:
    actions = []
    for action in iter_action_functions(model):
        actions.append(
            {
                "name": action.action_name,
                "symbol": action.qualified_name or action.name,
                "source": action.source,
                "returnType": {
                    "cpp": action.return_cpp_type,
                    "typescript": action.return_ts_type,
                },
                "params": [
                    {
                        "name": param.name,
                        "cppType": param.cpp_type,
                        "typescriptType": param.ts_type,
                        "optional": param.optional,
                    }
                    for param in action.params
                ],
            }
        )
    return json.dumps({"actions": actions}, ensure_ascii=False, indent=2) + "\n"


def parse_file(
    path: str,
    cwd: str,
    parse_all: bool,
    quickjs: bool,
    default_target: str,
) -> tuple[str, MetaModel]:
    text = read_text(path)
    file_model = parse_input_file(
        text,
        source_path=osp.relpath(path, cwd),
        parse_all=parse_all,
        quickjs=quickjs,
        default_target=default_target,
    )
    return path, file_model


def run(config: dict[str, Any]) -> int:
    set_env(config)
    files = collect_input_files(config)
    if not files:
        raise RuntimeError("no input files. Use --input or --input_dir")

    verbose = bool(config.get("verbose"))
    debug = bool(config.get("debug"))
    cwd = os.getcwd()
    parse_all = bool(config.get("all")) and not bool(config.get("marked_only"))
    quickjs = bool(config.get("quickjs"))
    default_target = str(config.get("target") or "")
    jobs = resolve_jobs(config, len(files))

    model = MetaModel()
    if jobs == 1:
        results = [
            parse_file(path, cwd, parse_all, quickjs, default_target)
            for path in files
        ]
    else:
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
            results = list(
                executor.map(
                    lambda path: parse_file(path, cwd, parse_all, quickjs, default_target),
                    files,
                )
            )

    for path, file_model in results:
        if verbose:
            print(
                f"{path}: classes={len(file_model.classes)} functions={len(file_model.functions)}",
                file=sys.stderr,
            )
        if debug and (file_model.classes or file_model.functions):
            print(render_json(file_model), file=sys.stderr)
        model.extend(file_model)

    output_format = str(config.get("format") or "")
    format_explicit = bool(config.get("_format_explicit"))
    if not output_format:
        output_hint = str(config.get("output") or config.get("output_name") or "")
        output_format = "json" if output_hint.endswith(".json") else "dts"

    if output_is_directory(config) and not format_explicit and not config.get("output_name"):
        output_dir = str(config.get("output_dir") or config.get("output"))
        write_text(osp.join(output_dir, "types.d.ts"), render_action_dts(model))
        write_text(osp.join(output_dir, "metadata.json"), render_action_metadata(model))
    else:
        content = render_action_metadata(model) if output_format == "json" else render_action_dts(model)
        output_path = resolve_output_path(config, output_format)

        if output_path:
            write_text(output_path, content)
        else:
            sys.stdout.write(content)

    if verbose:
        final_model = dedupe_model(model)
        print(
            f"metatool: parsed {len(files)} files, generated "
            f"{len(final_model.classes)} classes and {len(final_model.functions)} functions "
            f"with {jobs} job(s)",
            file=sys.stderr,
        )
    return 0


def main() -> int:
    try:
        config = merge_args_with_config(parse_arguments())
        return run(config)
    except Exception as error:
        print(f"metatool: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
