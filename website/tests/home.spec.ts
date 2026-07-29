import { expect, test } from '@playwright/test';

test('首页提供三个清晰且可聚焦的主入口', async ({ page }) => {
  await page.goto('./');

  await expect(page.getByRole('heading', { level: 1, name: /OpenEPD 4\.7/ })).toBeVisible();

  for (const label of ['开始开发', '浏览资料', '项目展示']) {
    const link = page.getByRole('link', { name: label, exact: true });
    await expect(link).toBeVisible();
    await link.focus();
    await expect(link).toBeFocused();
  }
});

test('移动端首页没有横向溢出', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('./');

  const dimensions = await page.evaluate(() => ({
    scrollWidth: document.documentElement.scrollWidth,
    clientWidth: document.documentElement.clientWidth,
  }));

  expect(dimensions.scrollWidth).toBeLessThanOrEqual(dimensions.clientWidth);
});
