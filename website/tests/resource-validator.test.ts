import { describe, expect, test } from 'bun:test';
import { validateResources } from '../scripts/validate-resources';

const validLocalResource = {
  id: 'display-spec',
  title: 'Display specification',
  category: 'display',
  hardwareRevision: 'E0470A01-AF-S',
  sourceType: 'vendor-authorized',
  sourceUrl: '',
  redistribution: 'authorized',
  localPath: '/resources/vendor/display/spec.pdf',
  sha256: 'a'.repeat(64),
  bytes: 100,
  notes: 'Authorized material',
};

describe('validateResources', () => {
  test('接受字段完整的本地授权资料', () => {
    expect(validateResources([validLocalResource])).toEqual([]);
  });

  test('拒绝缺少授权状态的资料', () => {
    const invalid = { ...validLocalResource, redistribution: '' };
    expect(validateResources([invalid])).toContain('display-spec: redistribution 不能为空');
  });

  test('拒绝同时声明外链和本地镜像', () => {
    const invalid = {
      ...validLocalResource,
      id: 'gt911',
      sourceType: 'external-link',
      sourceUrl: 'https://example.com/gt911.pdf',
      redistribution: 'not-mirrored',
    };

    expect(validateResources([invalid])).toContain('gt911: external-link 不能设置 localPath');
  });

  test('拒绝格式错误的 SHA-256', () => {
    const invalid = { ...validLocalResource, sha256: 'abc' };
    expect(validateResources([invalid])).toContain('display-spec: sha256 必须是 64 位小写十六进制');
  });
});
