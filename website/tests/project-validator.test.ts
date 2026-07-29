import { describe, expect, test } from 'bun:test';
import { validateProjects } from '../src/data/projects';

describe('项目展示数据', () => {
  test('三个初始条目具备展示和溯源所需字段', () => {
    const result = validateProjects();

    expect(result).toEqual([]);
  });
});
