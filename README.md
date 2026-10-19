# RL78/F24 MCU CAN device App

App 封裝完成後，可用瀏覽器開啟 `DefaultBuild/app_build_report.html`，核對實際 BIN 的
metadata、256-byte 位址、CRC／SHA-256 與完整 App 向量表。
欄位與 IRQ 修改說明見 [編譯摘要](https://released.github.io/slide_boot_loader_for_RL78/#device-app-build-report)。

此 App 由相鄰 `RL78_F24_Boot_loader_CAN` 驗證後啟動，起點 `0x2000`，不是獨立 reset image。
請先讀 [完整分區、接線與操作說明](../RL78_F24_Boot_loader_CAN/README.md)。
編輯 C header、加入 ISR、產表與 Tool 驗證，請依
[MCU CAN device 設定與建置](https://released.github.io/slide_boot_loader_for_RL78/#device-metadata-vector-workshop)。

- App 自己持有 `app_image_config.h`、`app_irq_handlers.h` 與 MTPJ 註冊的 ISR C 檔。
- 共用映像契約引用相鄰 Boot 的 `image_metadata_config.h`（`IMAGE_*`）；
  CAN 設定與 RAM 交接分別位於 `can_boot_config.h`、`boot_app_handoff.h`。
  App 發行設定不另外複製共用分區／CAN 數值。
- INFO 欄位讀取使用獨立編譯的 `image_metadata.c/.h`；Boot 也引用同一組模組。
  解碼與完整 metadata／CRC 驗證的分工見 [C 模組移植](https://released.github.io/slide_boot_loader_for_RL78/#device-metadata-parser)。
- `APP_IMAGE_VERSION_CONTROL` 預設 `0U`；有效已安裝 App 為 `1U` 時，一般更新限同版／新版。
  單次降版操作見 [Tool 版本政策](https://released.github.io/slide_isp_tool_for_RL78/#force-old-version)。
- 新普通 ISR 使用 `#pragma interrupt Name`，不可加 `vect=`；再加入本 App 的 C header。
- CS+ pre-build 產生本 App 的 `app_irq_table.inc`／`app_identity.h`／`ftable.inc`，由 `ftable_RL78_F24.asm` 組譯；不重做 Boot。
- 只編輯兩份 C header；生成的 INC／header／table 不手改。post-build 會拒絕與 C header 不同的已編譯 metadata。
- `DefaultBuild/flash_app_variable_CRC.hex` 為初裝 App；`*_update.bin` 為 Tool CAN TAB 更新。
  App 根目錄 `boot_app.hex` 可供 Boot／App 合併初裝。
  原始 MOT 含 device file 的實體記錄，初裝使用封裝後的 HEX。
- INFO／ENTER 控制使用 standard700/701、Classical500K；UART115200 只作診斷。
- INFO health page `1B` 在初始化完成及觀察到 timer IRQ 後回報 APP_READY；Tool 更新後自動確認。
- 診斷 UART 每秒輸出 `log(timer)-V1: counter`，counter 由零遞增；在 `app_runtime_log.h` 編輯 `APP_RUNTIME_LOG_TAG`，可選 `"V1"`／`"V2"`。標籤與 metadata 版本獨立，輸出在主迴圈執行。
- Boot／App 可使用同名 ISR；實體向量、AA／55 owner 與 App mapping 的規則見 [中斷所有權](https://released.github.io/slide_boot_loader_for_RL78/#device-irq-ownership)。
- CAN 250K／500K／1M 的 driver／metadata／配對建置步驟見 [Device CAN 速率](https://released.github.io/slide_boot_loader_for_RL78/#device-can-bitrate)。
- 500K 正常更新、CRC、App 啟動與 UART counter 已完成實板驗證，見 [更新結果](https://released.github.io/slide_isp_tool_for_RL78/#f24-device-update-example)；不同速率、其他 IRQ、取消及斷電分別驗證。

```powershell
..\RL78_F24_Boot_loader_CAN\build.ps1 -Kind app -AppProject .
```
