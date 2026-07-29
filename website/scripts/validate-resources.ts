import { createHash } from 'node:crypto';
import { existsSync, readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { parse } from 'yaml';

export interface ResourceRecord {
  id?: unknown;
  title?: unknown;
  category?: unknown;
  hardwareRevision?: unknown;
  sourceType?: unknown;
  sourceUrl?: unknown;
  redistribution?: unknown;
  localPath?: unknown;
  sha256?: unknown;
  bytes?: unknown;
  notes?: unknown;
}

function text(value: unknown): string {
  return typeof value === 'string' ? value.trim() : '';
}

export function validateResources(resources: ResourceRecord[]): string[] {
  const errors: string[] = [];
  const ids = new Set<string>();

  for (const resource of resources) {
    const id = text(resource.id) || '<unknown>';
    const sourceType = text(resource.sourceType);
    const localPath = text(resource.localPath);
    const sourceUrl = text(resource.sourceUrl);
    const redistribution = text(resource.redistribution);
    const sha256 = text(resource.sha256);

    if (ids.has(id)) errors.push(`${id}: id 不能重复`);
    ids.add(id);

    for (const field of ['title', 'category', 'hardwareRevision', 'sourceType', 'notes'] as const) {
      if (!text(resource[field])) errors.push(`${id}: ${field} 不能为空`);
    }

    if (!redistribution) errors.push(`${id}: redistribution 不能为空`);

    if (sourceType === 'external-link') {
      if (localPath) errors.push(`${id}: external-link 不能设置 localPath`);
      if (!sourceUrl) errors.push(`${id}: external-link 必须设置 sourceUrl`);
      continue;
    }

    if (!localPath) errors.push(`${id}: 本地资料必须设置 localPath`);
    if (!/^[a-f0-9]{64}$/.test(sha256)) {
      errors.push(`${id}: sha256 必须是 64 位小写十六进制`);
    }
    if (typeof resource.bytes !== 'number' || resource.bytes <= 0) {
      errors.push(`${id}: bytes 必须是正整数`);
    }
  }

  return errors;
}

export function validateLocalFiles(resources: ResourceRecord[], publicDirectory: string): string[] {
  const errors: string[] = [];

  for (const resource of resources) {
    if (text(resource.sourceType) === 'external-link') continue;

    const id = text(resource.id) || '<unknown>';
    const relativePath = text(resource.localPath).replace(/^\/+/, '');
    const absolutePath = join(publicDirectory, relativePath);
    if (!existsSync(absolutePath)) {
      errors.push(`${id}: 本地文件不存在 ${relativePath}`);
      continue;
    }

    const actualHash = createHash('sha256').update(readFileSync(absolutePath)).digest('hex');
    if (actualHash !== text(resource.sha256)) {
      errors.push(`${id}: 本地文件 SHA-256 不匹配`);
    }
  }

  return errors;
}

if (import.meta.main) {
  const scriptDirectory = dirname(fileURLToPath(import.meta.url));
  const websiteRoot = join(scriptDirectory, '..');
  const yamlPath = join(websiteRoot, 'src', 'data', 'resources.yml');
  const publicDirectory = join(websiteRoot, 'public');
  const document = parse(readFileSync(yamlPath, 'utf8')) as { resources?: ResourceRecord[] };
  const resources = document.resources ?? [];
  const errors = [
    ...validateResources(resources),
    ...validateLocalFiles(resources, publicDirectory),
  ];

  if (errors.length > 0) {
    for (const error of errors) console.error(`[resource-error] ${error}`);
    process.exit(1);
  }

  console.log(`[resource-ok] ${resources.length} entries validated`);
}
