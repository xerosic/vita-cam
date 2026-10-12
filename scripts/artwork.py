"""Generate Vita LiveArea assets. Requires Pillow; no downloaded artwork."""
from pathlib import Path
from math import sin, hypot
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]


def background(size):
    width, height = size
    image = Image.new("RGB", size)
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            t = y / height
            glow = max(0, 1 - hypot(x / width - .35, y / height - .15))
            pixels[x, y] = (int(6 + 15 * (1-t) + 7*glow), int(22 + 47*(1-t) + 28*glow), int(51 + 67*(1-t) + 34*glow))
    draw = ImageDraw.Draw(image)
    for i in range(4):
        points = [(x, height*.69 + sin(x/width*4+i*.24)*height*.12+i*height*.055) for x in range(width)]
        draw.line(points, fill=(31+i*3, 88+i*6, 137+i*7), width=1)
    return image


def lens(draw, x, y, scale):
    def box(rect):
        return tuple(int((x if i % 2 == 0 else y) + v*scale) for i, v in enumerate(rect))
    draw.rounded_rectangle(box((-40,-25,40,28)), radius=int(9*scale), fill="#dcecf4")
    draw.rounded_rectangle(box((-22,-36,13,-15)), radius=int(4*scale), fill="#dcecf4")
    draw.ellipse(box((-23,-22,23,24)), fill="#214e78", outline="#95c1dc", width=max(1,int(2*scale)))
    draw.ellipse(box((-18,-17,18,19)), fill="#081a34", outline="#4797bf", width=max(1,int(2*scale)))
    draw.ellipse(box((-11,-10,11,12)), fill="#123f68")
    draw.ellipse(box((-8,-9,1,0)), fill="#99e3f5")
    draw.rounded_rectangle(box((24,-17,33,-12)),radius=1,fill="#5c9ec1")


def save(image, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    image.quantize(colors=256).save(path, optimize=True)


def font(size):
    candidates = ["/System/Library/Fonts/Helvetica.ttc", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"]
    for path in candidates:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size=size)


icon = background((128,128))
draw = ImageDraw.Draw(icon)
draw.ellipse((5,5,123,123),outline="#78b7db",width=2)
lens(draw,64,67,1.08)
save(icon, ROOT / "assets/icon0.png")

bg = background((840,500))
draw = ImageDraw.Draw(bg)
draw.text((49,43),"Vita Camera",font=font(43),fill="#effaff")
lens(draw,420,276,1.75)
draw.text((51,444),"PHOTOGRAPHY",font=font(14),fill="#9bccdf")
save(bg, ROOT / "assets/livearea/bg.png")

startup = background((280,158))
draw = ImageDraw.Draw(startup)
lens(draw,70,78,1.05)
draw.text((125,51),"Vita",font=font(26),fill="#effaff")
draw.text((124,81),"Camera",font=font(26),fill="#effaff")
save(startup, ROOT / "assets/livearea/startup.png")
