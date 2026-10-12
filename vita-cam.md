# Camera capabilities and project scope

Vita Camera turns the PS Vita's built-in cameras into a compact still-photography workflow: direct controls, readable exposure aids, preserved originals and simple transfer. Its visual design follows the Vita era, while its processing retains the camera's low-resolution character.

For installation and everyday use, see the [README](README.md). Implementation details and measured behavior are maintained in [Architecture](docs/architecture.md) and [Device testing](docs/device-testing.md).

## Hardware reference

| Property | Published capability |
| --- | --- |
| Cameras | Front and rear |
| Maximum image size | 640 × 480 pixels, approximately 0.3 megapixels |
| VGA feed | 640 × 480 at 60 fps |
| QVGA feed | 320 × 240 at 120 fps |
| Reference model | Sony's published PCH-2000 specification |

Sony's [PCH-2000 specification](https://sonyinteractive.com/en/press-releases/2013/sony-computer-entertainment-to-introduce-new-slimmer-and-lighter-playstationvita-wi-fi-model-with-internal-memory-card/) lists these modes for both cameras. They describe camera-feed capabilities, not the frame rate of files saved by the stock Photos app. The 120 fps mode applies to QVGA.

The project does not rely on unverified aperture, focal length, minimum focus distance, sensor dimensions or component identity. Enlarging or cropping a VGA image cannot recover uncaptured detail.

## Capture model

The app captures 640 × 480 ABGR frames through VitaSDK. It prefers 30 fps, retaining 60 fps as a startup fallback. Device testing confirmed that the lower preference resolved dark indoor photographs; actual exposure duration remains unmeasured.

Three stages remain distinct:

1. **Camera settings** affect the image delivered by the device.
2. **App processing** applies an optional color look or digital crop.
3. **Export** writes the original, any edited copy and capture metadata.

The Original look adds no tone adjustment, denoising, grain or sharpening. Mirroring, guides, graphs and warning overlays are preview aids. Full-frame originals remain available beside edited photographs.

## Photographic controls

The [VitaSDK camera header](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h) defines the available modes. Runtime validation remains necessary because individual values can be rejected.

| Control | Project behavior |
| --- | --- |
| Exposure compensation | SDK steps spanning −2.0 to +2.0; Up/Down adjusts EV while shooting |
| ISO | Auto, 100, 200 and 400; metadata records the selected mode |
| White balance | Auto and SDK presets; unsupported values are skipped |
| Anti-flicker | Auto, 50 Hz and 60 Hz where accepted by the camera |
| Digital zoom | App crop from 1–4×, with a separate exported copy |
| Color looks | Original, Mono, Warm and Cool, implemented in app processing |

EV remains separate from image brightness processing. A successful setting call does not establish calibrated exposure or sensitivity. Anti-flicker is intended for pulsing artificial light; its effect depends on the light source and requires visual testing.

## Focus, exposure and RAW limits

The public camera interface exposes no autofocus trigger, manual lens position, aperture adjustment or direct shutter-duration control. Magnification can help inspect sharpness, but cannot refocus the lens.

The SDK declares a `RAW8` buffer format without establishing a documented photographic RAW workflow, Bayer pattern or ISP bypass. The app's original PNGs contain processed RGB pixels from the camera. Lossless encoding preserves those pixels; it does not turn them into sensor RAW data.

The stock [Photos application](https://manuals.playstation.net/document/en/psvita/photos/camera.html) has its own photo, video and panorama features. Those capabilities are separate from this app, which currently implements still capture and its own gallery.

## Product priorities

- Preserve native image proportions and the original camera output.
- Keep capture, EV, zoom and exposure aids accessible without navigating menus.
- Use concise labels, consistent spacing and restrained Vita-style surfaces.
- Keep preview processing responsive with graphs and warnings enabled.
- Preserve originals and make save failures recoverable while the app remains open.
- Make local transfer practical from a phone browser.

Luminance and RGB histograms, waveform and zebras describe the delivered image. They indicate tonal distribution and possible clipping; they do not measure untouched sensor data.

## Sharing boundaries

The implemented transfer path is a local HTTP gallery over Wi-Fi. Shared-network transfer to iPhone is confirmed. Personal Hotspot and offline access points are additional routes to validate; neither requires changing the photo-transfer protocol.

Sony's [Bluetooth documentation](https://manuals.playstation.net/document/en/psvita/settings/bluetooth.html) does not list a file-transfer profile. The app provides neither Bluetooth photo transfer nor AirDrop. USB transfer through VitaShell works with a computer; direct iPhone compatibility is unverified.

## Future work

These are candidates for investigation, not implemented features or release commitments.

| Area | Candidate | Validation needed |
| --- | --- | --- |
| Export | JPEG copies and native Photos integration | Preserve PNG originals; test encoding and import failures |
| Gallery | Favorites, selection and batch downloads | Keep browsing responsive as the library grows |
| Workflow | Saved camera presets and configurable controls | Preserve readable labels and predictable input behavior |
| Sequences | Interval shooting, burst and EV bracketing | Measure save throughput, frame identity and exposure settling |
| Camera processing | Noise reduction, sharpness and night-mode experiments | Compare repeatable scenes and native-size exports |
| Exposure lock | Investigate auto-control hold | Establish which automatic adjustments are held and restored |
| RAW8 | Characterize buffer output | Establish layout, meaning and reproducibility before offering export |

VitaSDK's auto-control hold and exposure-ceiling functions are sparsely documented. `ImageQuality` is not documented as a JPEG-quality setting. These controls should remain experimental until their behavior is established.

For each experiment, change one parameter at a time, record return values and settings, allow the image to settle, and compare exported files under repeatable lighting. Capture timestamps and frame identifiers help separate fresh frames from transitional ones.

Useful implementation references include the [VitaSDK camera sample](https://github.com/vitasdk/samples/blob/master/camera/src/main.c) and the [photo-export interface](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/photoexport.h).
