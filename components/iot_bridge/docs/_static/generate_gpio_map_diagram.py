#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
"""Generate ESP-IoT-Bridge GPIO map diagrams (SVG + PNG)."""

from __future__ import annotations

from pathlib import Path

OUT_DIR = Path(__file__).parent

CHIPS = ['ESP32', 'ESP32-S2', 'ESP32-S3', 'ESP32-C3', 'ESP32-C2', 'ESP32-C6', 'ESP32-H2']

# (gpio, signal_name, fixed) — empty signal_name shows GPIO number only
Pin = tuple[int, str, bool]
RowData = dict[str, list[Pin]]
TableRow = tuple[str, str, RowData]

TABLE_ROWS: list[TableRow] = [
    (
        'Button',
        '',
        {
            'ESP32': [(3, '', False)],
            'ESP32-S2': [(38, '', False)],
            'ESP32-S3': [(38, '', False)],
            'ESP32-C3': [(9, '', False)],
            'ESP32-C2': [(9, '', False)],
            'ESP32-C6': [(9, '', False)],
            'ESP32-H2': [(9, '', False)],
        },
    ),
    (
        'USB',
        '',
        {
            'ESP32': [],
            'ESP32-S2': [(19, 'USB_DM', True), (20, 'USB_DP', True)],
            'ESP32-S3': [(19, 'USB_DM', True), (20, 'USB_DP', True)],
            'ESP32-C3': [],
            'ESP32-C2': [],
            'ESP32-C6': [],
            'ESP32-H2': [],
        },
    ),
    (
        'Ethernet',
        'ETH_RMII',
        {
            'ESP32': [
                (0, 'RMII_CLK', True),
                (21, 'RMII_TX_EN', True),
                (19, 'RMII_TXD0', True),
                (22, 'RMII_TXD1', True),
                (27, 'RMII_CRS_DV', True),
                (25, 'RMII_RXD0', True),
                (26, 'RMII_RXD1', True),
            ],
            'ESP32-S2': [],
            'ESP32-S3': [],
            'ESP32-C3': [],
            'ESP32-C2': [],
            'ESP32-C6': [],
            'ESP32-H2': [],
        },
    ),
    (
        'Ethernet',
        'ETH_SMI',
        {
            'ESP32': [
                (23, 'SMI_MDC', False),
                (18, 'SMI_MDIO', False),
                (5, 'PHY_RESET', False),
            ],
            'ESP32-S2': [],
            'ESP32-S3': [],
            'ESP32-C3': [],
            'ESP32-C2': [],
            'ESP32-C6': [],
            'ESP32-H2': [],
        },
    ),
    (
        'Ethernet',
        'ETH_SPI',
        {
            'ESP32': [
                (14, 'ETH_SPI_CLK', False),
                (13, 'ETH_SPI_MOSI', False),
                (12, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-S2': [
                (17, 'ETH_SPI_CLK', False),
                (16, 'ETH_SPI_MOSI', False),
                (18, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-S3': [
                (17, 'ETH_SPI_CLK', False),
                (16, 'ETH_SPI_MOSI', False),
                (18, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-C3': [
                (6, 'ETH_SPI_CLK', False),
                (7, 'ETH_SPI_MOSI', False),
                (2, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-C2': [
                (6, 'ETH_SPI_CLK', False),
                (7, 'ETH_SPI_MOSI', False),
                (2, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-C6': [
                (6, 'ETH_SPI_CLK', False),
                (7, 'ETH_SPI_MOSI', False),
                (2, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
            'ESP32-H2': [
                (6, 'ETH_SPI_CLK', False),
                (7, 'ETH_SPI_MOSI', False),
                (2, 'ETH_SPI_MISO', False),
                (15, 'ETH_SPI_CS', False),
                (4, 'ETH_SPI_INT', False),
            ],
        },
    ),
    (
        'Modem',
        'UART',
        {
            'ESP32': [(32, 'MODEM_UART_TX', False), (33, 'MODEM_UART_RX', False)],
            'ESP32-S2': [],
            'ESP32-S3': [],
            'ESP32-C3': [(4, 'MODEM_UART_TX', False), (5, 'MODEM_UART_RX', False)],
            'ESP32-C2': [(4, 'MODEM_UART_TX', False), (5, 'MODEM_UART_RX', False)],
            'ESP32-C6': [(4, 'MODEM_UART_TX', False), (5, 'MODEM_UART_RX', False)],
            'ESP32-H2': [(4, 'MODEM_UART_TX', False), (5, 'MODEM_UART_RX', False)],
        },
    ),
    (
        'Modem',
        'USB',
        {
            'ESP32': [],
            'ESP32-S2': [(19, 'USB_DM', True), (20, 'USB_DP', True)],
            'ESP32-S3': [(19, 'USB_DM', True), (20, 'USB_DP', True)],
            'ESP32-C3': [],
            'ESP32-C2': [],
            'ESP32-C6': [],
            'ESP32-H2': [],
        },
    ),
    (
        'SPI',
        '',
        {
            'ESP32': [
                (15, 'SPI_CS', False),
                (14, 'SPI_CLK', False),
                (12, 'SPI_MISO', False),
                (13, 'SPI_MOSI', False),
                (2, 'SPI_HANDSHAKE', False),
                (6, 'SPI_DATA_READY', False),
            ],
            'ESP32-S2': [
                (10, 'SPI_CS', False),
                (12, 'SPI_CLK', False),
                (13, 'SPI_MISO', False),
                (11, 'SPI_MOSI', False),
                (2, 'SPI_HANDSHAKE', False),
                (6, 'SPI_DATA_READY', False),
            ],
            'ESP32-S3': [
                (10, 'SPI_CS', False),
                (12, 'SPI_CLK', False),
                (13, 'SPI_MISO', False),
                (11, 'SPI_MOSI', False),
                (2, 'SPI_HANDSHAKE', False),
                (6, 'SPI_DATA_READY', False),
            ],
            'ESP32-C3': [
                (10, 'SPI_CS', False),
                (6, 'SPI_CLK', False),
                (2, 'SPI_MISO', False),
                (7, 'SPI_MOSI', False),
                (3, 'SPI_HANDSHAKE', False),
                (4, 'SPI_DATA_READY', False),
            ],
            'ESP32-C2': [
                (10, 'SPI_CS', False),
                (6, 'SPI_CLK', False),
                (2, 'SPI_MISO', False),
                (7, 'SPI_MOSI', False),
                (3, 'SPI_HANDSHAKE', False),
                (4, 'SPI_DATA_READY', False),
            ],
            'ESP32-C6': [
                (10, 'SPI_CS', False),
                (6, 'SPI_CLK', False),
                (2, 'SPI_MISO', False),
                (7, 'SPI_MOSI', False),
                (3, 'SPI_HANDSHAKE', False),
                (4, 'SPI_DATA_READY', False),
            ],
            'ESP32-H2': [
                (10, 'SPI_CS', False),
                (6, 'SPI_CLK', False),
                (2, 'SPI_MISO', False),
                (7, 'SPI_MOSI', False),
                (3, 'SPI_HANDSHAKE', False),
                (4, 'SPI_DATA_READY', False),
            ],
        },
    ),
    (
        'SDIO',
        '',
        {
            'ESP32': [
                (15, 'SDIO_CMD', False),
                (14, 'SDIO_CLK', False),
                (2, 'SDIO_DAT0', False),
                (4, 'SDIO_DAT1', False),
                (12, 'SDIO_DAT2', False),
                (13, 'SDIO_DAT3', False),
            ],
            'ESP32-S2': [],
            'ESP32-S3': [],
            'ESP32-C3': [],
            'ESP32-C2': [],
            'ESP32-C6': [],
            'ESP32-H2': [],
        },
    ),
]

CN_TEXT = {
    'title': 'ESP-IoT-Bridge 默认 GPIO 映射',
    'subtitle': '各芯片接口默认引脚分配一览（可在 menuconfig 中修改）',
    'iface_col': '接口',
    'sub_col': '子项',
    'footnote': '红色标注的 GPIO 不可自定义',
}

EN_TEXT = {
    'title': 'ESP-IoT-Bridge Default GPIO Map',
    'subtitle': 'Default pin assignments per chip (configurable in menuconfig)',
    'iface_col': 'Interface',
    'sub_col': 'Sub',
    'footnote': 'GPIOs marked in red cannot be customized',
}

COL_IFACE = 92
COL_SUB = 92
COL_CHIP = 130
PAD_X = 28
PAD_TOP = 88
HEADER_H = 44
PIN_FONT = 8
LINE_H = 11
ROW_PAD = 15
WIDTH = PAD_X * 2 + COL_IFACE + COL_SUB + COL_CHIP * len(CHIPS)


def pin_lines(gpio: int, name: str) -> list[str]:
    if not name:
        return [f'GPIO{gpio}']
    one_line = f'GPIO{gpio} ({name})'
    if len(one_line) > 15:
        return [f'GPIO{gpio}', f'({name})']
    return [one_line]


def visual_line_count(pins: list[Pin]) -> int:
    if not pins:
        return 1
    return sum(len(pin_lines(gpio, name)) for gpio, name, _ in pins)


def row_height(pins_per_col: list[list[Pin]]) -> int:
    count = max((visual_line_count(p) for p in pins_per_col), default=1)
    return ROW_PAD * 2 + count * LINE_H


def chip_x(index: int) -> float:
    return PAD_X + COL_IFACE + COL_SUB + COL_CHIP * index + COL_CHIP / 2


def render_pins(pins: list[Pin], x: float, y: float) -> str:
    if not pins:
        return (
            f'<text x="{x:.1f}" y="{y:.1f}" text-anchor="middle" '
            f'font-size="11" fill="#a0aec0">—</text>'
        )
    parts = []
    first = True
    for gpio, name, fixed in pins:
        color = '#e53e3e' if fixed else '#2d3748'
        weight = '600' if fixed else '400'
        for line in pin_lines(gpio, name):
            dy = 0 if first else LINE_H
            first = False
            parts.append(
                f'<tspan x="{x:.1f}" dy="{dy}" font-size="{PIN_FONT}" '
                f'font-weight="{weight}" fill="{color}">{line}</tspan>'
            )
    return f'<text x="{x:.1f}" y="{y:.1f}" text-anchor="middle">{"".join(parts)}</text>'


def build_svg(t: dict, lang: str) -> str:
    font = (
        "'Segoe UI', 'PingFang SC', 'Microsoft YaHei', Arial, sans-serif"
        if lang == 'cn'
        else "'Segoe UI', Arial, sans-serif"
    )

    heights = [row_height([cols.get(chip, []) for chip in CHIPS]) for _, _, cols in TABLE_ROWS]
    table_h = sum(heights)
    height = PAD_TOP + HEADER_H + table_h + 56

    y = PAD_TOP
    header_bottom = y + HEADER_H
    table_w = COL_IFACE + COL_SUB + COL_CHIP * len(CHIPS)
    iface_x = PAD_X + COL_IFACE / 2
    sub_x = PAD_X + COL_IFACE + COL_SUB / 2

    # compute row geometry and section spans
    row_geoms: list[tuple[float, float]] = []
    row_y = header_bottom
    for rh in heights:
        row_geoms.append((row_y, rh))
        row_y += rh

    section_spans: dict[str, tuple[float, float]] = {}
    section_start: str | None = None
    section_top = 0.0
    for idx, (section, _, _) in enumerate(TABLE_ROWS):
        top, rh = row_geoms[idx]
        if section != section_start:
            if section_start is not None:
                section_spans[section_start] = (section_top, top)
            section_start = section
            section_top = top
    if section_start is not None:
        last_top, last_rh = row_geoms[-1]
        section_spans[section_start] = (section_top, last_top + last_rh)

    body: list[str] = [
        f'<rect x="{PAD_X}" y="{y}" width="{table_w}" height="{HEADER_H + table_h}" '
        f'rx="12" fill="#ffffff" stroke="#e2e8f0" stroke-width="1.5" filter="url(#shadow)"/>',
        f'<rect x="{PAD_X}" y="{y}" width="{table_w}" height="{HEADER_H}" '
        f'rx="12" fill="url(#hdrGrad)"/>',
        f'<rect x="{PAD_X}" y="{y + HEADER_H - 12}" width="{table_w}" height="12" fill="url(#hdrGrad)"/>',
    ]

    for section, (top, bottom) in section_spans.items():
        span_h = bottom - top
        body.append(
            f'<rect x="{PAD_X}" y="{top}" width="{COL_IFACE}" height="{span_h}" fill="#edf2f7"/>'
        )
        body.append(
            f'<text x="{iface_x:.1f}" y="{top + span_h / 2 + 5}" text-anchor="middle" '
            f'font-size="11.5" font-weight="700" fill="#2d3748">{section}</text>'
        )

    body.append(
        f'<text x="{iface_x:.1f}" y="{y + 28}" text-anchor="middle" font-size="12" '
        f'font-weight="700" fill="#ffffff">{t["iface_col"]}</text>'
    )
    body.append(
        f'<text x="{sub_x:.1f}" y="{y + 28}" text-anchor="middle" font-size="12" '
        f'font-weight="700" fill="#ffffff">{t["sub_col"]}</text>'
    )
    for i, chip in enumerate(CHIPS):
        body.append(
            f'<text x="{chip_x(i):.1f}" y="{y + 28}" text-anchor="middle" font-size="11.5" '
            f'font-weight="700" fill="#ffffff">{chip}</text>'
        )

    vx = PAD_X + COL_IFACE
    body.append(
        f'<line x1="{vx}" y1="{y + 8}" x2="{vx}" y2="{header_bottom - 8}" '
        f'stroke="#ffffff" stroke-opacity="0.25"/>'
    )
    vx += COL_SUB
    body.append(
        f'<line x1="{vx}" y1="{y + 8}" x2="{vx}" y2="{header_bottom - 8}" '
        f'stroke="#ffffff" stroke-opacity="0.25"/>'
    )
    for i in range(1, len(CHIPS)):
        vx = PAD_X + COL_IFACE + COL_SUB + COL_CHIP * i
        body.append(
            f'<line x1="{vx}" y1="{y + 8}" x2="{vx}" y2="{header_bottom - 8}" '
            f'stroke="#ffffff" stroke-opacity="0.25"/>'
        )

    for row_idx, (section, sub, cols) in enumerate(TABLE_ROWS):
        row_y, rh = row_geoms[row_idx]
        fill = '#f8fafc' if row_idx % 2 == 0 else '#ffffff'
        body.append(
            f'<rect x="{PAD_X + COL_IFACE}" y="{row_y}" width="{table_w - COL_IFACE}" '
            f'height="{rh}" fill="{fill}"/>'
        )
        body.append(
            f'<line x1="{PAD_X}" y1="{row_y + rh}" x2="{PAD_X + table_w}" y2="{row_y + rh}" '
            f'stroke="#e2e8f0" stroke-width="1"/>'
        )
        sub_label = sub if sub else '—'
        body.append(
            f'<text x="{sub_x:.1f}" y="{row_y + rh / 2 + 4}" text-anchor="middle" '
            f'font-size="10.5" font-weight="600" fill="#4a5568">{sub_label}</text>'
        )
        text_y = row_y + ROW_PAD + 11
        for i, chip in enumerate(CHIPS):
            body.append(render_pins(cols.get(chip, []), chip_x(i), text_y))

    for x_off in [COL_IFACE, COL_IFACE + COL_SUB]:
        x_line = PAD_X + x_off
        body.append(
            f'<line x1="{x_line}" y1="{header_bottom}" x2="{x_line}" y2="{row_y + rh}" '
            f'stroke="#e2e8f0" stroke-width="1"/>'
        )
    for i in range(1, len(CHIPS)):
        x_line = PAD_X + COL_IFACE + COL_SUB + COL_CHIP * i
        body.append(
            f'<line x1="{x_line}" y1="{header_bottom}" x2="{x_line}" y2="{row_y + rh}" '
            f'stroke="#e2e8f0" stroke-width="1"/>'
        )

    foot_y = row_geoms[-1][0] + row_geoms[-1][1] + 34
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{height}" viewBox="0 0 {WIDTH} {height}" font-family="{font}">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#f7fafc"/><stop offset="100%" stop-color="#edf2f7"/>
    </linearGradient>
    <linearGradient id="hdrGrad" x1="0%" y1="0%" x2="100%" y2="0%">
      <stop offset="0%" stop-color="#2b3645"/><stop offset="100%" stop-color="#3d4f63"/>
    </linearGradient>
    <filter id="shadow" x="-5%" y="-5%" width="110%" height="110%">
      <feDropShadow dx="0" dy="2" stdDeviation="4" flood-color="#000000" flood-opacity="0.10"/>
    </filter>
  </defs>
  <rect width="{WIDTH}" height="{height}" fill="url(#bg)"/>
  <text x="{WIDTH / 2:.1f}" y="38" text-anchor="middle" font-size="22" font-weight="700" fill="#1a202c">{t["title"]}</text>
  <text x="{WIDTH / 2:.1f}" y="62" text-anchor="middle" font-size="12" fill="#718096">{t["subtitle"]}</text>
  {"".join(body)}
  <circle cx="{PAD_X + 18}" cy="{foot_y - 4}" r="5" fill="#e53e3e"/>
  <text x="{PAD_X + 30}" y="{foot_y}" font-size="11" fill="#718096">{t["footnote"]}</text>
</svg>'''


def main() -> None:
    import re
    import subprocess

    pairs = [
        (CN_TEXT, 'cn', 'gpio_map.svg', 'gpio_map.png'),
        (EN_TEXT, 'en', 'gpio_map_en.svg', 'gpio_map_en.png'),
    ]
    for text, lang, svg_name, png_name in pairs:
        svg = build_svg(text, lang)
        svg_path = OUT_DIR / svg_name
        png_path = OUT_DIR / png_name
        svg_path.write_text(svg, encoding='utf-8')
        m = re.search(r'height="(\d+)"', svg)
        img_h = int(m.group(1)) if m else 900
        subprocess.run(
            [
                'google-chrome',
                '--headless',
                '--disable-gpu',
                f'--window-size={WIDTH},{img_h}',
                f'--screenshot={png_path}',
                f'file://{svg_path.resolve()}',
            ],
            check=True,
            capture_output=True,
        )
        print(f'Generated {svg_path.name} and {png_path.name}')


if __name__ == '__main__':
    main()
