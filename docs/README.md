# MachiUI Documentation Site

This directory contains the Astro Starlight documentation site for MachiUI.

## Local Development

```powershell
cd docs
pnpm install
pnpm dev
```

## Production Build

```powershell
cd docs
pnpm build
pnpm preview
```

## GitHub Pages Build Settings

`astro.config.mjs` reads two environment variables:

- `SITE`: the public GitHub Pages origin, such as `https://fabyday.github.io`.
- `BASE`: the repository base path, such as `/MachiUi`.

For this repository, the production documentation URL is:

```text
https://fabyday.github.io/MachiUi/
```

Build locally with the same values used by GitHub Actions:

```powershell
$env:SITE = "https://fabyday.github.io"
$env:BASE = "/MachiUi"
pnpm build
```
