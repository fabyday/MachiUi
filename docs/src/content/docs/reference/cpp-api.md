---
title: C++ API Documentation
description: How generated C++ API reference should connect to the Starlight docs.
sidebar:
  order: 1
---

The Starlight site should be the handwritten documentation layer: installation, architecture, runtime behavior, examples, and integration guides.

Generated C++ API reference should be produced separately from source comments. The repository already contains a root `Doxyfile` configured to scan `Source`.

## Recommended Flow

1. Keep conceptual documentation in `docs/src/content/docs`.
2. Add Doxygen comments to public C++ APIs as they stabilize.
3. Generate API reference with Doxygen during release or documentation builds.
4. Link the generated API output from this page.

## Current Doxygen Notes

The current `Doxyfile` is configured for XML output and writes generated files to `Docs`, which is ignored by Git. That is useful for local generated artifacts, but it is separate from the Starlight source directory named `docs`.

If the generated API reference should be published inside GitHub Pages later, use one of these approaches:

- Generate static HTML into `docs/public/api` before the Astro build.
- Generate XML and transform it into Starlight Markdown pages.
- Publish Doxygen HTML as a separate artifact and link to it from this site.

The first option is the simplest when the goal is a readable API reference with minimal custom tooling.
