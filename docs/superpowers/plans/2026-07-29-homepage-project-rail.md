# Homepage Project Rail Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将项目横向轨道放入首页首屏底部，确保桌面和移动端无需纵向滚动即可看到项目，并把资料区改成直接陈述。

**Architecture:** 新增只负责首页简版项目展示的 `HomeProjectRail.astro`，继续复用 `src/data/projects.ts`。首页用 `hero-stage` 将主视觉和项目轨道限制在一个 `100svh` 首屏中；轨道使用原生横向滚动与少量渐进增强脚本。

**Tech Stack:** Astro 7、TypeScript、CSS Scroll Snap、Lucide Astro、Playwright、Bun

---

### Task 1：锁定首屏项目轨道行为

**Files:**

- Modify: `website/tests/home.spec.ts`

- [ ] **Step 1：写桌面首屏失败测试**

新增以下测试，并把原有“三个入口”测试改为只要求开发与资料按钮：

```ts
test('桌面首屏直接显示可横向浏览的项目轨道', async ({ page }) => {
  await page.setViewportSize({ width: 1366, height: 768 });
  await page.goto('./');

  const rail = page.getByRole('region', { name: '首页项目展示' });
  const railBox = await rail.boundingBox();

  expect(railBox).not.toBeNull();
  expect(railBox!.y + railBox!.height).toBeLessThanOrEqual(768);
  await expect(rail.getByTestId('home-project-card')).toHaveCount(3);

  const track = rail.getByTestId('home-project-track');
  const before = await track.evaluate((element) => element.scrollLeft);
  await rail.getByRole('button', { name: '向右浏览项目' }).click();
  await expect.poll(() => track.evaluate((element) => element.scrollLeft)).toBeGreaterThan(before);
});
```

- [ ] **Step 2：写移动端和资料文案失败测试**

```ts
test('移动端首屏显示项目轨道且只有轨道内部横向滚动', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('./');

  const rail = page.getByRole('region', { name: '首页项目展示' });
  const railBox = await rail.boundingBox();
  const widths = await rail.getByTestId('home-project-track').evaluate((element) => ({
    client: element.clientWidth,
    scroll: element.scrollWidth,
    pageClient: document.documentElement.clientWidth,
    pageScroll: document.documentElement.scrollWidth,
  }));

  expect(railBox!.y + railBox!.height).toBeLessThanOrEqual(844);
  expect(widths.scroll).toBeGreaterThan(widths.client);
  expect(widths.pageScroll).toBeLessThanOrEqual(widths.pageClient);
});

test('资料区直接说明已整理资料', async ({ page }) => {
  await page.goto('./');

  await expect(page.getByRole('heading', { name: '已整理的硬件资料' })).toBeVisible();
  await expect(page.getByText('不是“文件堆”')).toHaveCount(0);
});
```

- [ ] **Step 3：运行测试并确认正确失败**

Run:

```powershell
Set-Location 'D:\project\openepd-47\.worktrees\website-mvp\website'
bunx playwright test tests/home.spec.ts
```

Expected: FAIL；首页尚无 `首页项目展示` 区域，资料标题仍为旧文案。

### Task 2：实现首页项目轨道组件

**Files:**

- Create: `website/src/components/HomeProjectRail.astro`
- Modify: `website/src/styles/site.css`
- Test: `website/tests/home.spec.ts`

- [ ] **Step 1：创建复用项目数据的组件**

组件必须直接导入共享数据，并输出具名区域、滚动轨道、三个卡片与左右控制：

```astro
---
import { ArrowUpRight, ChevronLeft, ChevronRight } from 'lucide-astro';
import { projects } from '../data/projects';

const base = import.meta.env.BASE_URL.replace(/\/$/, '');
const allProjects = `${base}/projects/`;
---

<section class="home-project-rail shell" aria-label="首页项目展示" data-project-rail>
  <header class="home-project-rail__header">
    <div>
      <span>PROJECTS / 横向浏览</span>
      <h2>项目展示</h2>
    </div>
    <div class="home-project-rail__actions">
      <a href={allProjects}>查看全部 <ArrowUpRight size={15} /></a>
      <button type="button" aria-label="向左浏览项目" data-project-prev>
        <ChevronLeft size={18} />
      </button>
      <button type="button" aria-label="向右浏览项目" data-project-next>
        <ChevronRight size={18} />
      </button>
    </div>
  </header>
  <div class="home-project-track" data-testid="home-project-track" data-project-track tabindex="0">
    {projects.map((project) => (
      <a
        class="home-project-card"
        data-testid="home-project-card"
        href={project.repository}
        style={`--project-accent:${project.accent}`}
      >
        <span>{project.categoryLabel} · {project.status}</span>
        <strong>{project.title}</strong>
        <small>{project.hardware} · {project.framework}</small>
        <ArrowUpRight size={17} aria-hidden="true" />
      </a>
    ))}
  </div>
</section>

<script>
  document.querySelectorAll<HTMLElement>('[data-project-rail]').forEach((rail) => {
    const track = rail.querySelector<HTMLElement>('[data-project-track]');
    const previous = rail.querySelector<HTMLButtonElement>('[data-project-prev]');
    const next = rail.querySelector<HTMLButtonElement>('[data-project-next]');
    if (!track || !previous || !next) return;

    const update = () => {
      previous.disabled = track.scrollLeft <= 1;
      next.disabled = track.scrollLeft + track.clientWidth >= track.scrollWidth - 1;
    };
    const move = (direction: number) => {
      const card = track.querySelector<HTMLElement>('[data-testid="home-project-card"]');
      track.scrollBy({ left: direction * (card?.offsetWidth ?? track.clientWidth * 0.8), behavior: 'smooth' });
    };

    previous.addEventListener('click', () => move(-1));
    next.addEventListener('click', () => move(1));
    track.addEventListener('scroll', update, { passive: true });
    update();
  });
</script>
```

- [ ] **Step 2：添加“轨道切入”样式**

在 `site.css` 中增加：

```css
.hero-stage {
  position: relative;
  min-height: 650px;
  height: calc(100svh - 86px);
  max-height: 900px;
  overflow: hidden;
}

.hero-stage .hero {
  height: 100%;
  min-height: 0;
  padding-block: clamp(34px, 5vh, 64px) 220px;
}

.home-project-rail {
  position: absolute;
  z-index: 4;
  right: 0;
  bottom: 18px;
  left: 0;
  height: 190px;
  border: 1px solid var(--ink);
  background: var(--paper-bright);
  box-shadow: 8px 8px 0 rgb(16 35 27 / 15%);
}

.home-project-track {
  display: flex;
  overflow-x: auto;
  gap: 12px;
  padding: 0 14px 14px;
  scroll-behavior: smooth;
  scroll-snap-type: x mandatory;
  scrollbar-width: thin;
}

.home-project-card {
  position: relative;
  flex: 0 0 clamp(390px, 38vw, 500px);
  min-height: 118px;
  padding: 15px 48px 14px 16px;
  border: 1px solid var(--line);
  border-top: 3px solid var(--project-accent);
  background: var(--paper);
  scroll-snap-align: start;
  text-decoration: none;
}
```

移动端断点必须将轨道改为约 230 px 高、卡片宽度 `82vw`，并隐藏大型 `.hero-visual`：

```css
@media (max-width: 720px) {
  .hero-stage {
    min-height: 758px;
    height: calc(100svh - 86px);
  }

  .hero-stage .hero {
    display: block;
    padding: 30px 0 238px;
  }

  .hero-stage .hero-visual {
    display: none;
  }

  .home-project-rail {
    bottom: 12px;
    height: 218px;
  }

  .home-project-card {
    flex-basis: 82vw;
  }
}
```

- [ ] **Step 3：暂时把组件接入首页验证测试能定位组件**

在 `index.astro` 导入组件，并用 `hero-stage` 包住现有 hero 与组件。运行：

```powershell
bunx playwright test tests/home.spec.ts
```

Expected: 组件数量、横向滚动测试开始通过；资料文案测试仍失败。

### Task 3：完成首页重排与直接资料文案

**Files:**

- Modify: `website/src/pages/index.astro`
- Modify: `website/src/styles/site.css`
- Test: `website/tests/home.spec.ts`

- [ ] **Step 1：完成首屏结构**

结构改为：

```astro
<section class="hero-stage">
  <div class="hero shell">
    <!-- 保留 hero-copy 与 hero-visual -->
  </div>
  <HomeProjectRail />
</section>
```

删除 hero 中独立的“项目展示”文字链接；保留“开始开发”和“浏览资料”按钮。

- [ ] **Step 2：删除重复区块**

从 `index.astro` 删除：

- `entryPoints` 数据和整个 `entry-section`
- 页面末尾 `showcase-section`
- 只被以上区块使用的图标导入

从 `site.css` 删除只服务于 `entry-section` 和旧 `showcase-section` 的样式。

- [ ] **Step 3：替换资料区文案**

```astro
<p class="eyebrow">AVAILABLE MATERIALS / 现有资料</p>
<h2>已整理的硬件资料</h2>
<p>
  这里包括 E0470A01-AF-S 规格书、结构图、GT911 触摸资料和 TPS65185 电源资料。
  每项都标注型号、来源、再分发状态和 SHA‑256。
</p>
```

- [ ] **Step 4：运行首页测试确认全绿**

Run:

```powershell
bunx playwright test tests/home.spec.ts
```

Expected: 所有首页测试 PASS。

- [ ] **Step 5：提交实现**

```powershell
git add website/src/components/HomeProjectRail.astro website/src/pages/index.astro website/src/styles/site.css website/tests/home.spec.ts
git commit -m "feat(site): surface projects in homepage hero"
```

### Task 4：完整验证与重新部署

**Files:**

- Modify: `docs/superpowers/plans/2026-07-29-homepage-project-rail.md`

- [ ] **Step 1：运行全部本地验证**

```powershell
Set-Location 'D:\project\openepd-47\.worktrees\website-mvp\website'
bun run check
bun run test:unit
bun run test:e2e
bun run build
```

Expected:

- Astro check：0 errors
- unit：5 tests passed
- Playwright：所有测试通过
- build：13 pages built

- [ ] **Step 2：推送分支并创建 PR**

```powershell
git push -u origin codex/home-project-strip
gh pr create --base main --head codex/home-project-strip
```

- [ ] **Step 3：等待 CI、合并并验证 Pages**

```powershell
gh pr checks --watch
gh pr merge --merge
gh run list --workflow pages.yml --limit 1
```

Expected: PR CI 和 Pages workflow 均为 `success`。

- [ ] **Step 4：线上浏览器验收**

检查：

- 1366×768 和 390×844 首屏可见项目轨道。
- 左右控制和原生横向滚动有效。
- 首页根元素无横向溢出。
- 资料区使用直接陈述。
- 控制台无 error/warn。
