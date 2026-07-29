# OpenEPD 4.7 Website / 网站

Astro + Starlight 静态网站，包含项目首页、开发文档、可追踪资料下载与社区项目展示。

## Local development / 本地开发

```powershell
Set-Location 'D:\project\openepd-47\website'
bun install --frozen-lockfile
bun run dev
```

本地站点路径：<http://localhost:4321/openepd-47/>

## Verification / 验证

```powershell
bun run check
bun run test:unit
bun run test:e2e
bun run build
```

生产站点由 GitHub Actions 部署到：
<https://exomind-team.github.io/openepd-47/>
