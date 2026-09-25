# nrf91-multi-image-fota-example

Two independently updatable application images on the nRF9151DK, behind a
two-stage bootloader (b0 + MCUboot).

- **app0**: MCUboot image 0, the main sysbuild image (repo root).
- **app1**: MCUboot image 1, added as a `SECONDARY_APP` sysbuild image
  (`app1/`).

Both apps confirm themselves, connect to LTE, then wait for a button:

- **Button 1** downloads `CONFIG_UPDATE_FILE_APP0` into image 0.
- **Button 2** downloads `CONFIG_UPDATE_FILE_APP1` into image 1.

Once the download finishes, the app reboots. MCUboot boots the image that has
the pending swap. Otherwise it boots app0, or app1 when Button 2 is held at
reset (`mcuboot_hooks/boot_hooks.c`).

Server settings are in `common/Kconfig`: `CONFIG_UPDATE_HOST`, the two file
names and `CONFIG_UPDATE_SEC_TAG`. Set them in both `prj.conf` and
`app1/prj.conf`. The files to upload are
`build/nrf-two-app/zephyr/zephyr.signed.bin` (app0) and
`build/app1/zephyr/zephyr.signed.bin` (app1).

## Workspace

`west.yml` pins the `savosaicic/sdk-nrf` fork that adds the multi-image
FOTA in `fota_download`.

### Setup

```
mkdir nrf-two-app-ws && cd nrf-two-app-ws
python3 -m venv .venv
source .venv/bin/activate
pip install west
west init -m https://github.com/savosaicic/nrf91-multi-image-fota-example.git --mr main
west update
pip install -r zephyr/scripts/requirements.txt
```

### Build and flash

```
west build -b nrf9151dk/nrf9151/ns -p
west flash
```

## Layout (`dts/partitions.dtsi`)

| Region          | Internal flash                        | External flash   |
| --------------- | ------------------------------------- | ---------------- |
| b0 (NSIB)       | 0x00000 (32 kB)                       |                  |
| MCUboot s0 / s1 | 0x08000 / 0x18000 (64 kB each)        |                  |
| app0 (image 0)  | slot0 at 0x28000 (416 kB, TF-M 32 kB) | slot1 at 0x00000 |
| app1 (image 1)  | slot2 at 0x90000 (416 kB, TF-M 32 kB) | slot3 at 0x68000 |

TF-M and imgtool read `slot0_*` from each image's own devicetree. app1
therefore includes the dtsi with `APP1_VIEW`: the layout is the same, but the
`slot0_*` labels are on app1's slot. The secondary labels are the same in both
views, so `fota_download` with `img_num = N` always writes `slot<2N+1>`.
