import { expect, test } from '@playwright/test';

test('首页提供清晰且可聚焦的开发与资料入口', async ({ page }) => {
  await page.goto('./');

  await expect(page.getByRole('heading', { level: 1, name: /OpenEPD 4\.7/ })).toBeVisible();

  for (const label of ['开始开发', '浏览资料']) {
    const link = page.getByRole('link', { name: label, exact: true });
    await expect(link).toBeVisible();
    await link.focus();
    await expect(link).toBeFocused();
  }
});

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
  await expect
    .poll(() => track.evaluate((element) => element.scrollLeft))
    .toBeGreaterThan(before);
});

test('较矮桌面窗口仍在首屏显示项目轨道', async ({ page }) => {
  await page.setViewportSize({ width: 1024, height: 600 });
  await page.goto('./');

  const railBox = await page
    .getByRole('region', { name: '首页项目展示' })
    .boundingBox();

  expect(railBox).not.toBeNull();
  expect(railBox!.y + railBox!.height).toBeLessThanOrEqual(600);
});

test('移动端首屏显示项目轨道且只有轨道内部横向滚动', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('./');

  const rail = page.getByRole('region', { name: '首页项目展示' });
  const railBox = await rail.boundingBox();
  const track = rail.getByTestId('home-project-track');
  const trackBox = await track.boundingBox();
  const projectCards = rail.getByTestId('home-project-card');
  await expect(projectCards).toHaveCount(3);
  const nextCardBox = await projectCards.nth(1).boundingBox();
  const widths = await track.evaluate((element) => ({
    client: element.clientWidth,
    scroll: element.scrollWidth,
    pageClient: document.documentElement.clientWidth,
    pageScroll: document.documentElement.scrollWidth,
  }));

  expect(railBox).not.toBeNull();
  expect(trackBox).not.toBeNull();
  expect(nextCardBox).not.toBeNull();
  expect(railBox!.y + railBox!.height).toBeLessThanOrEqual(844);
  expect(nextCardBox!.x).toBeLessThan(trackBox!.x + trackBox!.width);
  expect(widths.scroll).toBeGreaterThan(widths.client);
  expect(widths.pageScroll).toBeLessThanOrEqual(widths.pageClient);
});

test('资料区直接说明已整理资料', async ({ page }) => {
  await page.goto('./');

  await expect(page.getByRole('heading', { name: '已整理的硬件资料' })).toBeVisible();
  await expect(page.getByText('不是“文件堆”')).toHaveCount(0);
});
