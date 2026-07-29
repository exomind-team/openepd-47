import { expect, test } from '@playwright/test';

test('项目展示页提供筛选与 GitHub 提交入口', async ({ page }) => {
  await page.goto('./projects/');

  await expect(page.getByRole('heading', { level: 1, name: '项目展示' })).toBeVisible();
  await expect(page.getByTestId('project-card')).toHaveCount(3);

  await page.getByRole('button', { name: '参考工程' }).click();
  await expect(page.getByTestId('project-card').filter({ visible: true })).toHaveCount(1);

  const submission = page.getByRole('link', { name: '提交你的项目' });
  await expect(submission).toHaveAttribute(
    'href',
    'https://github.com/exomind-team/openepd-47/issues/new?template=project-submission.yml',
  );
});

test('项目展示页在移动端没有横向溢出', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('./projects/');

  const dimensions = await page.evaluate(() => ({
    scrollWidth: document.documentElement.scrollWidth,
    clientWidth: document.documentElement.clientWidth,
  }));

  expect(dimensions.scrollWidth).toBeLessThanOrEqual(dimensions.clientWidth);
});
