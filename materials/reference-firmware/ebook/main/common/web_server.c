#include "web_server.h"
#include "event_bus.h"
#include "book_storage.h"
#include "image_storage.h"
#include "app_album.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#define TAG "WEB_SERVER"

// 简单的 HTML 上传页面，使用 JS 发送纯文件内容和处理图片抖动
static const char* upload_html = 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>ESP32 E-Ink 控制中心</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>body{font-family:sans-serif;margin:20px;text-align:center;background:#f0f0f0;} "
    ".card{background:white;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1);margin-bottom:20px;} "
    "button{padding:10px 20px;font-size:16px;background:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;} "
    "input{margin:10px;} canvas{display:none;}</style></head><body>"
    "<h2>ESP32 E-Ink 控制中心</h2>"
    "<div class=\"card\">"
    "<h3>传书中心 (电子书)</h3>"
    "<p>请选择一个纯文本小说文件 (必须是 UTF-8 编码的 .txt 文件)</p>"
    "<input type=\"file\" id=\"bookInput\" accept=\".txt\">"
    "<br><button onclick=\"uploadBook()\">上传电子书</button>"
    "</div>"
    "<div class=\"card\">"
    "<h3>相册中心 (图片)</h3>"
    "<p id=\"imgUsage\">加载中...</p>"
    "<ul id=\"imgList\" style=\"list-style:none;padding:0;text-align:left;max-width:420px;margin:0 auto;\"></ul>"
    "<p>请选择要显示的照片，系统会自动进行裁剪和灰阶抖动处理</p>"
    "<input type=\"file\" id=\"imageInput\" accept=\"image/*\">"
    "<br><button id=\"uploadBtn\" onclick=\"uploadImage()\">处理并上传照片</button>"
    "<canvas id=\"canvas\"></canvas>"
    "</div>"
    "<p id=\"status\" style=\"color:green;font-weight:bold;\"></p>"
    "<script>"
    "function showStatus(msg) { document.getElementById('status').innerText = msg; }"
    "function refreshImageList() {"
    "  fetch('/api/images').then(r => r.json()).then(data => {"
    "    document.getElementById('imgUsage').innerText ="
    "      '已用 ' + data.count + '/' + data.max + ' 张 · 剩余 ' + data.free_kb + ' KB';"
    "    const list = document.getElementById('imgList');"
    "    list.innerHTML = '';"
    "    data.files.forEach(f => {"
    "      const li = document.createElement('li');"
    "      li.style.margin = '8px 0';"
    "      const btn = document.createElement('button');"
    "      btn.innerText = '删除';"
    "      btn.style.marginLeft = '12px';"
    "      btn.onclick = () => deleteImage(f);"
    "      li.appendChild(document.createTextNode(f));"
    "      li.appendChild(btn);"
    "      list.appendChild(li);"
    "    });"
    "    document.getElementById('uploadBtn').disabled = data.count >= data.max;"
    "  }).catch(() => { document.getElementById('imgUsage').innerText = '无法读取相册列表'; });"
    "}"
    "function deleteImage(name) {"
    "  if (!confirm('确定删除 ' + name + ' ?')) return;"
    "  showStatus('正在删除...');"
    "  fetch('/delete_image?name=' + encodeURIComponent(name), { method: 'POST' })"
    "    .then(r => r.text().then(t => { if (!r.ok) throw new Error(t); return t; }))"
    "    .then(t => { showStatus(t); refreshImageList(); })"
    "    .catch(err => showStatus('删除失败：' + err.message));"
    "}"
    "window.onload = refreshImageList;"
    "function uploadBook() {"
    "  const file = document.getElementById('bookInput').files[0];"
    "  if (!file) { alert('请先选择TXT文件！'); return; }"
    "  showStatus('正在上传电子书，请耐心等待...');"
    "  const url = '/upload?name=' + encodeURIComponent(file.name);"
    "  fetch(url, { method: 'POST', body: file })"
    "    .then(response => response.text())"
    "    .then(text => showStatus(text))"
    "    .catch(err => showStatus('上传失败：' + err));"
    "}"
    "function uploadImage() {"
    "  const file = document.getElementById('imageInput').files[0];"
    "  if (!file) { alert('请先选择图片文件！'); return; }"
    "  showStatus('正在本地处理图片...');"
    "  const img = new Image();"
    "  img.onload = function() {"
    "    const canvas = document.getElementById('canvas');"
    "    const ctx = canvas.getContext('2d');"
    "    const TARGET_W = 684, TARGET_H = 1216;"
    "    canvas.width = TARGET_W; canvas.height = TARGET_H;"
    "    ctx.fillStyle = 'white'; ctx.fillRect(0, 0, TARGET_W, TARGET_H);"
    "    let scale = Math.max(TARGET_W/img.width, TARGET_H/img.height);"
    "    let nw = img.width * scale, nh = img.height * scale;"
    "    let dx = (TARGET_W - nw)/2, dy = (TARGET_H - nh)/2;"
    "    ctx.drawImage(img, dx, dy, nw, nh);"
    "    let imgData = ctx.getImageData(0, 0, TARGET_W, TARGET_H);"
    "    let data = imgData.data;"
    "    let gray = new Uint8Array(TARGET_W * TARGET_H);"
    "    for (let i = 0; i < data.length; i += 4) {"
    "      gray[i/4] = data[i]*0.299 + data[i+1]*0.587 + data[i+2]*0.114;"
    "    }"
    "    for (let y = 0; y < TARGET_H; y++) {"
    "      for (let x = 0; x < TARGET_W; x++) {"
    "        let idx = y * TARGET_W + x;"
    "        let oldPixel = gray[idx];"
    "        let newPixel = Math.round(oldPixel / 17) * 17;"
    "        gray[idx] = newPixel;"
    "        let err = oldPixel - newPixel;"
    "        if (x < TARGET_W - 1) gray[idx + 1] += err * 7 / 16;"
    "        if (y < TARGET_H - 1) {"
    "          if (x > 0) gray[idx + TARGET_W - 1] += err * 3 / 16;"
    "          gray[idx + TARGET_W] += err * 5 / 16;"
    "          if (x < TARGET_W - 1) gray[idx + TARGET_W + 1] += err * 1 / 16;"
    "        }"
    "      }"
    "    }"
    "    let packed = new Uint8Array(TARGET_W * TARGET_H / 2);"
    "    for (let i = 0; i < gray.length; i += 2) {"
    "      packed[i/2] = ((gray[i] / 17) << 4) | (gray[i+1] / 17);"
    "    }"
    "    let magic = new TextEncoder().encode('EI02');"
    "    let out = new Uint8Array(magic.length + packed.length);"
    "    out.set(magic); out.set(packed, magic.length);"
    "    showStatus('图片处理完成，正在上传...');"
    "    let outName = file.name.replace(/\\.[^.]+$/, '') + '.raw';"
    "    fetch('/upload_image?name=' + encodeURIComponent(outName), {"
    "      method: 'POST',"
    "      headers: { 'Content-Type': 'application/octet-stream' },"
    "      body: out"
    "    })"
    "      .then(r => r.text().then(t => { if (!r.ok) throw new Error(t); return t; }))"
    "      .then(text => { showStatus(text); refreshImageList(); })"
    "      .catch(err => showStatus('上传失败：' + err.message));"
    "  };"
    "  img.src = URL.createObjectURL(file);"
    "}"
    "</script></body></html>";

static esp_err_t index_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received GET request for index page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, upload_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t upload_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Receiving file... Content-Length: %d bytes", req->content_len);

    char dest[128];
    char query[128];
    const char* upload_name = "book.txt";

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char name[64];
        if (httpd_query_key_value(query, "name", name, sizeof(name)) == ESP_OK && name[0]) {
            upload_name = name;
        }
    }

    if (!book_storage_make_upload_path(upload_name, dest, sizeof(dest))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "无效的文件名");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saving book to %s", dest);

    FILE *fd = fopen(dest, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char buf[1024];
    int received = 0;
    int remaining = req->content_len;
    
    // 如果 Content-Length 为 0，可能走的是 chunked，我们给个足够大的 remaining
    if (remaining == 0) {
        remaining = 10 * 1024 * 1024; // 10MB
    }

    // 使用可靠的 remaining 递减读取法
    while (remaining > 0) {
        int ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; // 超时重试
            }
            ESP_LOGE(TAG, "File reception failed or aborted!");
            fclose(fd);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (ret == 0) {
            ESP_LOGW(TAG, "Connection closed prematurely by client or finished.");
            break;
        }
        
        fwrite(buf, 1, ret, fd);
        remaining -= ret;
        received += ret;
        
        // 每收到 200KB 打印一下进度
        if (received % (1024 * 200) < sizeof(buf)) {
            ESP_LOGI(TAG, "Received %d bytes...", received);
        }
    }

    fflush(fd); // 确保强制写入磁盘
    fclose(fd);
    ESP_LOGI(TAG, "File upload complete! Total received: %d bytes", received);
    
    event_post(EVT_BOOK_UPLOADED);
    
    httpd_resp_sendstr(req, "上传成功！墨水屏正在自动刷新并加载您的新书，请稍候...");
    return ESP_OK;
}

static esp_err_t send_plain_error(httpd_req_t* req, const char* msg)
{
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, msg);
}

static size_t spiffs_free_bytes(void)
{
    size_t total = 0;
    size_t used = 0;
    if (esp_spiffs_info(NULL, &total, &used) != ESP_OK) {
        return 0;
    }
    return (total > used) ? (total - used) : 0;
}

static esp_err_t receive_album_body(httpd_req_t* req, FILE* fd, int expect, const char* dest_path)
{
    char buf[4096];
    int received = 0;
    bool bounded = (req->content_len > 0);
    int remaining = bounded ? req->content_len : (expect + 4096);

    while (true) {
        if (bounded && remaining <= 0) {
            break;
        }

        int chunk = (int)sizeof(buf);
        if (bounded && chunk > remaining) {
            chunk = remaining;
        }

        int ret = httpd_req_recv(req, buf, chunk);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (ret < 0) {
            ESP_LOGE(TAG, "Album recv failed: %d", ret);
            fclose(fd);
            if (dest_path) {
                remove(dest_path);
            }
            return ESP_FAIL;
        }
        if (ret == 0) {
            break;
        }

        size_t written = fwrite(buf, 1, ret, fd);
        if (written != (size_t)ret) {
            ESP_LOGE(TAG, "Album fwrite failed: %s", strerror(errno));
            fclose(fd);
            if (dest_path) {
                remove(dest_path);
            }
            return ESP_FAIL;
        }

        if (bounded) {
            remaining -= ret;
        }
        received += ret;

        if (!bounded && received > expect + 4096) {
            ESP_LOGW(TAG, "Album upload exceeds expected size, stopping");
            break;
        }
    }

    fflush(fd);
    fclose(fd);

    if (received < ALBUM_PACKED_BYTES / 2) {
        ESP_LOGE(TAG, "Album too short: %d bytes", received);
        if (dest_path) {
            remove(dest_path);
        }
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Album upload complete! Total received: %d bytes", received);
    return ESP_OK;
}

static esp_err_t upload_image_to_path(httpd_req_t* req, const char* dest)
{
    int expect = req->content_len;
    if (expect <= 0) {
        expect = ALBUM_FILE_BYTES;
    }

    char err_msg[128];
    if (!image_storage_can_upload_path(dest)) {
        if (image_storage_count() >= IMAGE_MAX_SLOTS) {
            return send_plain_error(req, "相册已满，请先删除部分图片后再上传");
        }
        return send_plain_error(req, "无法上传该图片，请检查存储空间");
    }

    struct stat st;
    bool exists = (stat(dest, &st) == 0);
    int space_need = exists ? (ALBUM_FILE_BYTES * 2) : expect;
    if (!image_storage_prepare_space(space_need, err_msg, sizeof(err_msg))) {
        return send_plain_error(req, err_msg);
    }

    FILE* fd = fopen(dest, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to open %s: %s", dest, strerror(errno));
        snprintf(err_msg, sizeof(err_msg), "无法创建图片文件: %s", strerror(errno));
        return send_plain_error(req, err_msg);
    }

    if (receive_album_body(req, fd, expect, dest) != ESP_OK) {
        return send_plain_error(req, "图片数据不完整，请重新上传");
    }

    event_post(EVT_IMAGE_CHANGED);
    event_post(EVT_ALBUM_UPLOADED);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "照片上传成功！");
    return ESP_OK;
}

static esp_err_t upload_image_post_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "Receiving image upload... Content-Length: %d", req->content_len);

    char dest[128];
    char query[128];
    const char* upload_name = "photo.raw";

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char name[64];
        if (httpd_query_key_value(query, "name", name, sizeof(name)) == ESP_OK && name[0]) {
            upload_name = name;
        }
    }

    if (!image_storage_make_upload_path(upload_name, dest, sizeof(dest))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "无效的文件名");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saving image to %s", dest);
    return upload_image_to_path(req, dest);
}

static esp_err_t upload_album_post_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "Legacy /upload_album called");

    char dest[128];
    char auto_name[48];
    snprintf(auto_name, sizeof(auto_name), "photo_%lld.raw",
             (long long)(esp_timer_get_time() / 1000));

    if (!image_storage_make_upload_path(auto_name, dest, sizeof(dest))) {
        return send_plain_error(req, "无法生成图片路径");
    }

    return upload_image_to_path(req, dest);
}

static esp_err_t delete_image_post_handler(httpd_req_t* req)
{
    char query[128];
    char name[64] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK
        || httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK
        || !name[0]) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "缺少 name 参数");
        return ESP_FAIL;
    }

    if (!image_storage_delete_by_name(name)) {
        return send_plain_error(req, "删除失败，文件不存在或无效");
    }

    event_post(EVT_IMAGE_CHANGED);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "图片已删除");
    return ESP_OK;
}

static esp_err_t api_images_get_handler(httpd_req_t* req)
{
    char paths[IMAGE_MAX_SLOTS][64];
    int count = image_storage_scan(paths, IMAGE_MAX_SLOTS);

    size_t total = 0;
    size_t used = 0;
    esp_spiffs_info(NULL, &total, &used);
    size_t free_bytes = spiffs_free_bytes();

    char json[1200];
    int pos = snprintf(json, sizeof(json),
                       "{\"max\":%d,\"count\":%d,\"free_kb\":%u,\"used_kb\":%u,\"files\":[",
                       IMAGE_MAX_SLOTS, count,
                       (unsigned)(free_bytes / 1024),
                       (unsigned)((used + 1023) / 1024));

    for (int i = 0; i < count && pos > 0 && pos < (int)sizeof(json) - 64; i++) {
        const char* name = strrchr(paths[i], '/');
        if (name) {
            name++;
        } else {
            name = paths[i];
        }

        pos += snprintf(json + pos, sizeof(json) - (size_t)pos, "%s\"%s\"",
                        (i > 0) ? "," : "", name);
    }

    if (pos > 0 && pos < (int)sizeof(json) - 4) {
        snprintf(json + pos, sizeof(json) - (size_t)pos, "]}");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}
void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // 增加最大请求 URI 长度和栈大小，确保稳定
    config.max_uri_handlers = 12;
    config.stack_size = 12288;
    config.recv_wait_timeout = 30;
    config.send_wait_timeout = 30;
    ESP_LOGI(TAG, "Starting Web Server on port %d...", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = index_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_post = {
            .uri      = "/upload",
            .method   = HTTP_POST,
            .handler  = upload_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_post);
        httpd_uri_t uri_post_album = {
            .uri      = "/upload_album",
            .method   = HTTP_POST,
            .handler  = upload_album_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_post_album);

        httpd_uri_t uri_post_image = {
            .uri      = "/upload_image",
            .method   = HTTP_POST,
            .handler  = upload_image_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_post_image);

        httpd_uri_t uri_delete_image = {
            .uri      = "/delete_image",
            .method   = HTTP_POST,
            .handler  = delete_image_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_delete_image);

        httpd_uri_t uri_api_images = {
            .uri      = "/api/images",
            .method   = HTTP_GET,
            .handler  = api_images_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_api_images);

        ESP_LOGI(TAG, "Web Server started successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to start Web Server!");
    }
}

void spiffs_init(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 24,
      .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    book_storage_init();
    image_storage_init();
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}