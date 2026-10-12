# Deep Dive into PS Vita Camera Capabilities

Implementation: [Vita Camera](README.md), a native C++17/VitaSDK photo app. See [architecture](docs/architecture.md) and [device checks](docs/device-testing.md) for implemented features and remaining hardware validation. Experimental capture modes remain future work.

# PS Vita cameras: surprisingly fast, deliberately low-resolution

**The PS Vita’s cameras are better understood as gaming sensors than as conventional cameras.** Both front and rear cameras top out at **640 × 480**, but Sony specifies camera-feed rates of **60 fps at that resolution** and **120 fps at 320 × 240**. Sony explicitly says responsive gameplay and augmented reality drove the design—not high-resolution photography. [23][37]

The most important distinction throughout this deep dive is **what the camera hardware can deliver, what software can access, and what the built-in Photos app actually saves**. Those are not interchangeable.

## 1. Hardware capabilities

| Feature | Capability |
|---|---|
| Cameras | One front-facing, one rear-facing |
| Maximum published resolution | **640 × 480 — VGA**, commonly described as 0.3 megapixels |
| High-resolution feed mode | **640 × 480 at 60 fps** |
| High-speed feed mode | **320 × 240 at 120 fps** |
| Original PCH-1000 | Published camera specifications above |
| Slim PCH-2000 | Same published resolution and frame-rate specifications |
| Design priority | Responsive gameplay and augmented reality |

Sony’s original announcement specifications and its official Slim specification sheet agree on these camera modes. The Slim therefore **does not offer a published camera-resolution or frame-rate upgrade** over the original. That does not establish that every internal camera component is identical. [17][37]

### The rear camera is not a higher-resolution “main” camera

Sony lists the front and rear cameras together under the same performance specification. There is no documented rear-camera resolution advantage comparable to the front/rear split on many phones. Both can take photos and videos in the Photos app. [5][37]

### What the 120 fps specification means—and does not mean

The **120 fps figure applies at QVGA, 320 × 240**, not VGA. It describes a camera-feed capability; it does **not** by itself establish that the Photos app saves 120 fps videos or provides slow-motion recording. [37]

The developer interface separately exposes resolution and frame-rate settings, reinforcing that software chooses a capture configuration. However, an API listing of resolutions and frame rates is not proof that every possible combination works. [1][22]

**Practical implication:** don’t buy a Vita expecting a ready-made 120 fps slow-motion camera.

## 2. What the built-in Photos app can do

Sony’s manual documents more functionality than the modest resolution might suggest.

### Still photos and ordinary video

You can:

- Use **either camera** for photographs or video.
- Switch between photo and video modes.
- Use the touchscreen shutter or the physical **R shoulder button**.
- Change image size through the app’s size-switching control.
- Open the most recent capture directly from its thumbnail.
- Hide and restore the interface by tapping the screen.
- Record a video lasting **up to 30 minutes per clip**. [5]

The manual also states that the shutter sound cannot be muted, have its volume adjusted, or be replaced with another sound. [7]

### Panorama capture

The Photos app includes a dedicated **panoramic camera** with selectable panoramic modes. It guides you through shooting, lets you delete the last shot in the panorama sequence, and saves the result under Camera Images. The R button works here too. [19]

This is an important distinction: a composite panoramic image can cover a wider scene, but it is not evidence of a higher-resolution camera sensor.

### Geotagging

Photos can include location information when location services are enabled and a location is obtained. Sony documents positioning using Wi-Fi access points, with GPS satellites and cellular base stations additionally used on **3G/Wi-Fi models**. [5]

For privacy, turn off **Settings → Location Data → Use Location Data** before shooting if you do not want your location embedded in photos. [5]

## 3. Video recording: the easily misunderstood part

### Camera feed versus saved video

There are three different stages:

1. **Camera capture:** the sensor supplies images.
2. **Application processing:** software reads, displays, or analyzes those images.
3. **File recording:** software encodes and saves a video.

Sony’s **60 fps VGA / 120 fps QVGA specification describes the camera capability**. Its Photos manual documents video recording and the 30-minute limit, but does not specify the saved file’s exact frame rate, codec, bitrate, or complete recording-mode matrix. [5][37]

**I could not establish the stock recorder’s exact encoding settings from a reliable primary source or an original recorded file.** Consequently, I would not present either “it records at 60 fps” or the frequently repeated “it is capped at 29.97/30 fps” as a verified universal specification.

If the saved-video details matter for your project, export a recording from the actual Vita and inspect it with a media-analysis tool. That resolves the file properties rather than inferring them from the hardware specification.

### Video recording was added through firmware

Sony’s **February 20, 2012** announcement for firmware **1.61** lists video capture in the Photo application among the features received by first-time updaters. It also distinguishes users who had already installed **1.60**, for whom 1.61 provided further system improvements. This explains why launch-era descriptions and later descriptions of the Photos app can differ. [38]

### Playback formats are not capture formats

Sony lists JPEG, TIFF, BMP, GIF, PNG and MPO among files the **Photos application can view**, along with videos taken using Photos. For MPO, it displays one representative image. [36]

That list does **not** mean the camera can save photographs in all those formats, nor that the Vita shoots stereoscopic MPO photographs. Similarly, a console’s supported video-playback codecs do not establish the camera recorder’s encoding settings.

## 4. Image quality: what the specifications imply

### Resolution is the fundamental ceiling

At **640 × 480**, there is little room for cropping or enlarging a photograph while retaining fine detail. Software scaling can make the image larger, but cannot supply scene detail the camera never captured. Sony’s own explanation places the emphasis on responsive gameplay rather than high-quality still photography. [23][37]

My assessment is that the cameras make sense for:

- Small, deliberately low-resolution snapshots.
- Capturing a face or an object for a game.
- Turning real-world imagery into game decoration.
- Experiments with a fast, low-resolution camera feed.

They are a poor fit for detailed document photography, substantial cropping, or conventional high-quality video production.

### Why low resolution made sense for games

A game may need to locate an AR marker or incorporate a player’s face without requiring a detailed photograph. Sony specifically describes the cameras as designed for AR gameplay, including face mapping and virtual objects placed on real surfaces. [24]

**Engineering interpretation:** exchanging image detail for a faster, smaller feed is a sensible trade-off for those tasks. Sony confirms the gameplay priority, though its public statements do not quantify end-to-end tracking latency. [23][24]

### Optics and sensor identity: avoid false precision

I did not find dependable primary documentation establishing the Vita cameras’ exact:

- Lens aperture.
- Focal length or field of view.
- Minimum focusing distance.
- Sensor dimensions or pixel pitch.
- Sensor part number across hardware revisions.

Those omissions matter if you are planning close-up photography, optical modifications, or component replacement. I would not substitute specifications from the separate PSP camera accessory or other PlayStation cameras.

The Vita teardown does show a removable rear-camera assembly and a modular internal design, but that is not sufficient evidence for a higher-resolution replacement or cross-generation camera compatibility. [11]

## 5. Games: where the cameras become genuinely interesting

### Augmented reality and face capture

Sony gives two clear examples:

- **Reality Fighters:** mapping your face onto a fighter.
- **Table Soccer:** placing a virtual soccer game on a real-world coffee table.

These illustrate two different uses: capturing appearance and interpreting the camera view as a space for gameplay. [24]

### Tearaway

*Tearaway* uses the real cameras through its **Quantum Lens**, allowing photographs of the real world using either the front or rear camera. [25]

There is a subtle but important trap here: the game also offers macro, zoom, high-speed and wide-angle lenses for its **virtual in-game camera**. Those are game mechanics—not physical optical upgrades to the Vita’s cameras. [25]

### LittleBigPlanet PS Vita

Sony’s creation demonstration showed a developer photographing someone’s mouth and placing that photograph into a level as its goal. The camera thus becomes an **asset-creation tool**, not merely a way to take standalone photos. [26]

**My pick for understanding the cameras’ creative value:** explore *Tearaway* or *LittleBigPlanet* rather than judging the hardware solely through ordinary snapshots.

## 6. Developer and homebrew access

The public **VitaSDK** camera interface exposes substantially more than the stock Photos controls.

### Capture resolutions

Its resolution enumeration includes:

- 640 × 480
- 640 × 360
- 480 × 272
- 352 × 288
- 320 × 240
- 176 × 144
- 160 × 120. [1]

The frame-rate enumeration includes modes ranging from **3 fps to 120 fps**, including 15, 20, 30 and 60 fps. Again, these declarations are not a validated table of all supported resolution/frame-rate combinations. [1][22]

### Frame-buffer formats

The interface declares:

- Planar and packed **YUV 4:2:2**.
- Planar **YUV 4:2:0**.
- **ARGB** and **ABGR**.
- A format named **RAW8**. [31]

RAW8 being present in an API is not equivalent to the stock camera offering photographic RAW-file capture.

### Camera controls

Documented functions include:

| Control category | Exposed interface |
|---|---|
| Stream lifecycle | Open, start, read, stop and close |
| Exposure | Exposure compensation, ISO and gain |
| Color | White balance and saturation |
| Lighting | Anti-flicker, backlight compensation and night mode |
| Framing | Zoom control |

These are supported by the public camera headers and documentation. Their existence does not guarantee that every setting behaves identically in every mode or revision. The headers themselves mark some less-understood controls with question marks. [3][30][32][33]

This makes the Vita interesting for custom preview software, image-processing experiments and game-camera prototypes. It does not remove the hardware’s published VGA ceiling. [37]

### “Vita as a webcam”: an important caveat

The well-known **vita-udcd-uvc** plugin makes the Vita behave like a USB Video Class device, but its documented purpose is to stream the **Vita’s screen framebuffer**, not directly expose its physical cameras. [29]

Therefore:

- Its listed **1280 × 720** output is not evidence of a 720p Vita camera.
- A camera app shown on the screen and then streamed would still be screen capture.
- USB display streaming and direct sensor capture are different capabilities. [29]

## 7. Saving, exporting and using your captures

Sony documents transfer of photos and camera videos between the Vita and a **computer or PS3 using Content Manager**. In the Photos gallery, select the item and use **Options → Copy**. The manual limits copying videos through this Photos workflow to videos taken using Photos. [9]

The Photos application has a documented limit of **10,000 files**, although available storage and individual file sizes can prevent you from reaching it. [36]

For practical experimentation, I recommend this workflow:

1. Take a few stills and a short video with each camera.
2. Include a bright scene, an indoor scene and a moving subject.
3. Export the originals.
4. Compare the files on the same display.
5. Inspect actual dimensions and video metadata before drawing conclusions about recording modes.

That distinguishes hardware limitations, app processing and the appearance of the Vita’s own screen.

## Bottom line

**The PS Vita has low-resolution cameras with unusually fast published feed modes, built around interactive play.** The Slim retains the original’s published camera capabilities; neither is an upgrade path for higher-resolution capture. [17][37]

Its strongest uses are **AR, face capture, creative game assets and developer experimentation**. Its weakest use is conventional detailed photography. Most importantly, **do not confuse the 120 fps sensor specification, a game’s virtual lenses, video-playback support, or USB screen-streaming resolutions with what the physical cameras actually record.** [23][25][29][36][37]

### Sources

1. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
3. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
5. [Taking photos and videos](https://manuals.playstation.net/document/en/psvita/photos/camera.html)
7. [Taking photos and videos](https://manuals.playstation.net/document/en/psvita/photos/camera.html)
9. [Getting the most out of your photos and videos](https://manuals.playstation.net/document/en/psvita/photos/share.html)
11. [PlayStation Vita Teardown](https://ifixit.com/Teardown/PlayStation+Vita+Teardown/7872)
17. [Sony Announces PlayStation Vita, Revolutionary Portable Entertainment Device](https://techpowerup.com/147035/sony-announces-playstation-vita-revolutionary-portable-entertainment-device?cp=1)
19. [Taking photos and videos](https://manuals.playstation.net/document/en/psvita/photos/camera.html)
22. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
23. [PS Vita: The Ultimate FAQ](https://blog.playstation.com/2012/02/16/ps-vita-the-ultimate-faq)
24. [PS Vita: The Ultimate FAQ](https://blog.playstation.com/2012/02/16/ps-vita-the-ultimate-faq)
25. [How Tearaway’s in-game photography works](https://blog.playstation.com/archive/2013/08/29/how-tearaways-in-game-photography-works)
26. [LittleBigPlanet for PS Vita: A Touch of Creation](https://blog.playstation.com/2011/08/22/littlebigplanet-for-ps-vita-a-touch-of-creation)
29. [GitHub - xerpi/vita-udcd-uvc: PSVita UDCD USB Video Class plugin](https://github.com/xerpi/vita-udcd-uvc)
30. [vita-headers/include/psp2/camera.h at master · vitasdk/vita-headers](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h)
31. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
32. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
33. [vita-headers/include/psp2/camera.h at master · vitasdk/vita-headers](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h)
36. [Types of files you can view with the Photos application](https://manuals.playstation.net/document/en/psvita/photos/filetypes.html)
37. [Sony Computer Entertainment to Introduce New Slimmer and Lighter PlayStation®Vita Wi-Fi Model with Internal Memory Card](https://sonyinteractive.com/en/press-releases/2013/sony-computer-entertainment-to-introduce-new-slimmer-and-lighter-playstationvita-wi-fi-model-with-internal-memory-card)
38. [PS Vita System Software Update (v1.61)](https://blog.playstation.com/2012/02/20/ps-vita-system-software-update-v1-61)

---

## You

i really like how the photo taken with it looks. I want to develop a custom, professional-ish photo app for the vita. In that direction, what does it have to offer?

---

## Ariados

**Yes—there’s a worthwhile app here.** If you like the Vita’s pictures already, I’d approach it as **a serious photographic interface for a distinctive camera**, rather than an attempt to make it produce modern-phone images.

The useful surprise is that homebrew can access considerably more camera controls than the stock Photos app exposes. The challenge is separating genuinely useful controls from poorly understood settings.

I looked into the public camera headers and the actual VitaSDK camera sample. Here’s what they offer for your project.

## 1. Photographic controls you can build around

The public interface has these controls, and the sample demonstrates calls to several of them. **A declared setting is not necessarily a calibrated photographic control**: the sample itself comments that gain “doesn’t seem to work.” Build your interface around measured behavior, not just available function names. [1][14][15]

| Feature | What you can access | What I’d do with it |
|---|---|---|
| **Exposure compensation** | `sceCameraSetEV`; sample uses −20 to +20 | Make this the main shooting adjustment |
| **ISO selection** | Auto, 100, 200, 400 | Offer after testing whether it changes exposure/noise consistently |
| **White balance** | Auto and presets including daylight, cool-white fluorescent and Standard Light Source A | Useful for consistent colors and deliberate color casts |
| **Brightness and contrast** | Separate setters, used in the sample | Put in a “look” panel, separate from exposure |
| **Saturation** | Setter and documented levels | Useful for restrained color recipes |
| **Sharpness** | Documented strength settings, plus a separate sharpness-off function | Test carefully; potentially important to preserving the look |
| **Camera effects** | Effect setter, exercised in the sample | Optional; not where I’d start |
| **Night mode** | Several modes labeled by lighting conditions | Treat as experimental until you characterize its behavior |
| **Anti-flicker** | Auto, 50 Hz and 60 Hz modes | Useful under artificial lighting |
| **Backlight compensation** | On/off | Optional automatic assistance |

These capabilities come from the headers and sample—not proof that every combination behaves as expected on every unit. [1][8][9][14][15]

### Exposure compensation deserves priority

For this app, I would make **EV adjustment faster than opening a menu**: D-pad left/right, or a thumbstick with discrete steps.

Keep exposure compensation separate from brightness. The interface exposes distinct functions for them, and the sample adjusts them separately. Your testing should establish how each changes highlights, shadows and noise rather than presenting them as interchangeable. [14]

That’s the sort of improvement that could make the Vita feel like a purposeful camera without changing its character.

### White balance could be a major creative feature

The interface offers preset white-balance modes rather than a documented Kelvin-temperature control. [1]

My recommendation:

- Auto for casual shooting.
- Clearly named presets for consistency.
- Saved recipes combining WB, EV and color settings.
- A quick before/after comparison.

A **“warm indoor” recipe** is a reasonable app feature. A numerical **“4,300 K” setting** would be misleading unless you implement and characterize your own color transformation.

## 2. The most interesting controls are also the least documented

These functions exist:

```c
sceCameraSetAutoControlHold(...)
sceCameraSetExposureCeiling(...)
sceCameraSetImageQuality(...)
sceCameraSetNoiseReduction(...)
sceCameraSetSharpnessOff(...)
```

But the header marks auto-control hold and exposure ceiling with question marks, and provides no explanatory behavior or parameter ranges for the last three. [8][9]

### Auto-control hold: investigate early

This is the obvious candidate for an **exposure/color lock**, but the documentation does not establish precisely which automatic processes it freezes. [9]

I would test it like this:

1. Let the camera settle on a stationary scene.
2. Enable hold.
3. Introduce a bright object, then a strongly colored object.
4. Observe whether brightness and color balance remain stable.
5. Repeat for each camera and selected WB/ISO mode.

If it works reliably, a physical-button lock would be one of the app’s best features. **Name the control according to what your test demonstrates**, rather than assuming it is a complete AE/AWB lock.

### Noise reduction and sharpness-off: especially relevant to your taste

There are separate functions for noise reduction and disabling sharpness. That is more promising than merely turning the documented sharpness strength down. [8][11]

However, don’t assume “off” automatically produces the look you prefer. Your favorite stock photos may owe part of their appearance to the camera’s existing processing.

I’d include three experimental profiles:

- **Baseline:** leave processing settings untouched.
- **Texture:** investigate reduced noise reduction.
- **Soft:** investigate disabled sharpening.

Compare original-size exports, not just the live view. Promote these to named shooting styles only after you know what they actually do.

**Also, `ImageQuality` is not documented as JPEG quality.** Don’t connect it to a “JPEG quality” slider without establishing its meaning. [8]

## 3. What “professional-ish” cannot honestly mean here

### Not full manual exposure

The public camera interface does not document a direct shutter-duration or aperture control. It offers EV, ISO/gain-related settings and the poorly described exposure-ceiling function instead. [1][9][14]

So I would not initially advertise:

- Manual shutter speeds.
- Aperture priority.
- Bulb exposure.
- A conventional fully manual “M” mode.

An interface built around **automatic exposure with useful overrides** is much more defensible.

### Not focus control

The public camera interface has no documented focus adjustment. [8][9][14]

Accordingly, touch-to-focus and manual-focus sliders should not be part of the initial scope. A magnified preview could still help users judge sharpness, but it would be an inspection tool—not lens control.

### Not guaranteed photographic RAW

`SCE_CAMERA_FORMAT_RAW8` exists, described only as “8 bit raw data.” The same interface offers YUV, ARGB and ABGR output formats. [12]

That does **not** establish that RAW8 gives you usable Bayer data, a known color-filter arrangement, or an ISP-bypassed photographic negative.

I’d make RAW8 a research branch, not a promised launch feature. Likewise, saving an ordinary processed frame as PNG should be called **lossless export**, not RAW.

## 4. Preserve the appearance before adding features

**Your first milestone should be reproducing the stock camera’s look closely enough that you still enjoy the pictures.**

I’d build a deliberately boring initial capture mode:

- Native-sized frames.
- No added denoising.
- No artificial grain.
- No automatic enhancement.
- No forced upscaling.
- Original aspect ratio.
- No overlays baked into the photograph.

Then compare stock-app and custom-app captures under repeatable lighting. This tests whether different defaults, output formats, color conversion or file encoding alter the look you like.

### Separate three kinds of change

Keep these distinct in your architecture:

1. **Camera settings:** affect the delivered frame.
2. **App processing:** optional color/tone changes after capture.
3. **Export encoding:** how the result is stored.

The camera provides processed RGB and YUV-format options; you do not have to start by decoding an unknown RAW stream. The VitaSDK sample uses ABGR-format capture. [12][14]

My preferred save model would be:

- **Original:** the frame before your app’s creative processing.
- **Edited version:** recipe applied.
- **Settings sidecar:** requested camera settings, app version and processing recipe.

Call those settings *requested* unless you can verify the actual exposure values. A selected ISO mode is not a complete EXIF exposure measurement.

## 5. You can add serious shooting tools without better optics

These are features I would prioritize because they improve the photographic experience rather than trying to “fix” the image:

### Composition and exposure aids

Implement in the app:

- Rule-of-thirds and square-composition guides.
- Live luminance histogram.
- Highlight-clipping overlay or zebras.
- Shadow-clipping warning.
- Uncluttered fullscreen preview.
- Image review with native-pixel magnification.
- Explicit front-camera mirroring options.

The camera sample already demonstrates streaming capture and uses the camera-reverse setting, giving you a starting point for orientation testing. [14][15]

A histogram or zebra display should describe **the delivered image**, not pretend to measure untouched sensor saturation.

### Shooting modes

Good candidates:

- Self-timer.
- Interval shooting.
- Burst capture.
- EV bracketing.
- Preset/recipe recall.
- Contact sheets and favorites.

`SceCameraRead` includes a frame identifier and timestamp, which are useful for distinguishing delivered frames in a capture pipeline. [2]

**Bracketing needs settling time.** Don’t change EV and immediately assume the next read is a properly exposed bracket frame. Measure the delay, then discard transitional frames.

For your aesthetic, I would leave HDR merging and multi-frame denoising out of the default path. Those can change precisely the roughness and tonal behavior you enjoy.

## 6. A practical implementation foundation

Assuming you mean a **homebrew app**, I’d start with **C/C++ and VitaSDK**, using its camera sample as the hardware-access reference.

The sample:

- Allocates a CDRAM memory block.
- Supplies a capture buffer through `SceCameraInfo`.
- Opens and starts a camera.
- Uses ABGR output.
- Exercises exposure, color and other controls. [14][15]

That is a useful starting point, **not a production-ready camera app**. Its experimental comments are a reason to build a cleaner layer around it.

### Recommended architecture

**Camera service**
- Owns camera lifecycle and buffers.
- Validates requested modes.
- Checks setter return codes.
- Logs settings and errors.
- Tracks frame identity.

**Live-view renderer**
- Displays the frame without distortion.
- Draws overlays separately.
- Computes preview analysis at a manageable rate.

**Capture queue**
- Copies the selected frame into an app-owned buffer.
- Keeps that image stable while the live camera continues.
- Hands encoding and saving to a worker.

**Image/export layer**
- Applies optional recipes.
- Saves original and edited variants.
- Stores settings metadata.
- Handles storage errors without losing the shot.

The key design decision: **don’t let the camera overwrite a frame while you are encoding it.**

### Native Photos integration

VitaSDK exposes `scePhotoExportFromFile` and `scePhotoExportFromData`, with a parameter structure supporting a photo title and app-related descriptive fields. [13]

I would first save reliably into the app’s own storage, then add native photo export as a separate feature. That keeps capture success independent of gallery integration.

## 7. The development sequence I’d choose

### First: a camera laboratory

Before polishing the interface, make a build that:

- Shows every current setting.
- Logs return codes and getter results.
- Captures numbered comparison images.
- Changes one parameter at a time.
- Saves a settings record beside each picture.
- Can reset to baseline.

Prioritize testing:

1. EV.
2. WB presets.
3. ISO.
4. Auto-control hold.
5. Noise reduction.
6. Sharpness-off.
7. Frame-rate/night-mode interactions.

The interface declares low-rate modes—including entries described as 3.75 and 7.5 fps—but that alone does not prove longer exposures or better low-light photographs. Measure what happens before labeling a mode “slow shutter.” [12]

### Then: a small, excellent photo app

My suggested first release:

- Clean native-aspect live view.
- Fast EV adjustment.
- WB presets.
- R-button shutter.
- Timer.
- Histogram and optional zebras.
- Original-preserving export.
- Last-shot review.
- A few tested recipes.
- Lock control only if its behavior is established.

### Later: experimental modes

- Burst and bracket sequences.
- Interval shooting.
- Alternative processing profiles.
- RAW8 investigation.
- Multi-frame processing as an explicit optional mode.

**My strongest recommendation:** make it feel like a well-designed compact camera, not a miniature Lightroom. The compelling product is *“the Vita pictures I love, with control, consistency and a great shooting workflow.”* You don’t need to eliminate its limitations to make that genuinely useful.

### Sources

1. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/camera.h)
2. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/camera.h)
8. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/camera.h)
9. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/camera.h)
11. [◆ SceCameraInfo](https://docs.vitasdk.org/group__SceCameraUser.html)
12. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/camera.h)
13. [Untitled source](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/photoexport.h)
14. [Untitled source](https://raw.githubusercontent.com/vitasdk/samples/master/camera/src/main.c)
15. [Untitled source](https://raw.githubusercontent.com/vitasdk/samples/master/camera/src/main.c)
