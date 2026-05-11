# Deploying to GitHub Pages

MachiUI docs are built from the `docs/` directory and deployed to:

```text
https://fabyday.github.io/MachiUi/
```

The repository workflow lives at `.github/workflows/deploy-docs.yml`.

```yaml
name: Deploy Docs to GitHub Pages

on:
  push:
    branches: [main]
    paths:
      - 'docs/**'
      - '.github/workflows/deploy-docs.yml'
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: pages
  cancel-in-progress: false

jobs:
  build:
    name: Build Starlight docs
    runs-on: ubuntu-latest
    steps:
      - name: Checkout repository
        uses: actions/checkout@v6

      - name: Install, build, and upload docs
        uses: withastro/action@v6
        with:
          path: docs
          node-version: 24
          package-manager: pnpm@10.32.1
        env:
          SITE: https://fabyday.github.io
          BASE: /MachiUi

  deploy:
    name: Deploy to GitHub Pages
    needs: build
    runs-on: ubuntu-latest
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - name: Deploy artifact
        id: deployment
        uses: actions/deploy-pages@v5
```

## GitHub Repository Settings

After pushing the workflow:

1. Open `https://github.com/fabyday/MachiUi`.
2. Go to **Settings -> Pages**.
3. Under **Build and deployment**, set **Source** to **GitHub Actions**.
4. Push to `main`, or run **Deploy Docs to GitHub Pages** manually from the Actions tab.

## Important Notes

- `SITE` must be `https://fabyday.github.io`.
- `BASE` must be `/MachiUi` because this is a project page, not the root `fabyday.github.io` repository.
- Commit `docs/pnpm-lock.yaml` so the workflow installs the same dependency graph.
- Do not commit `docs/node_modules`, `docs/dist`, or `docs/.astro`.
- If the default branch is not `main`, update the workflow branch filter.
