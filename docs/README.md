# MachiUI Documentation Site

This directory contains the English Astro Starlight documentation site for MachiUI.
The root locale is explicitly configured as `en` in `astro.config.mjs`.

## Local Development

```sh
cd docs
pnpm install --frozen-lockfile
pnpm dev
```

## Production Build

```sh
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

On macOS or Linux, set the same production values with:

```sh
SITE=https://fabyday.github.io BASE=/MachiUi pnpm build
```

The installation guide covers Windows and macOS. The Metal backend page documents
its current support and limitations. Updating these sources does not deploy the
site until the GitHub Pages workflow runs.
