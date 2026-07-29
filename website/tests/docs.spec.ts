import { expect, test } from '@playwright/test';

const docs = [
  ['./quick-start/', '快速开始'],
  ['./driver/build/', '构建环境'],
  ['./hardware/display/', '显示屏'],
  ['./hardware/gt911/', 'GT911 触摸'],
  ['./hardware/power/', 'TPS65185 电源'],
  ['./faq/', '常见问题'],
] as const;

for (const [url, heading] of docs) {
  test(`${heading} 文档可访问`, async ({ page }) => {
    await page.goto(url);
    await expect(page.getByRole('heading', { level: 1, name: heading })).toBeVisible();
  });
}

test('GT911 文档引用乐鑫官方组件且不误写 FT5446U 驱动', async ({ page }) => {
  await page.goto('./hardware/gt911/');

  await expect(page.getByRole('link', { name: /乐鑫组件仓库/ })).toHaveAttribute(
    'href',
    'https://components.espressif.com/components/espressif/esp_lcd_touch_gt911',
  );
  await expect(page.getByText('FT5446U 驱动', { exact: true })).toHaveCount(0);
});
