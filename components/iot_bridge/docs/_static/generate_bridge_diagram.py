#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
"""Generate ESP-IoT-Bridge architecture diagrams (SVG + PNG)."""

from pathlib import Path

OUT_DIR = Path(__file__).parent
HEIGHT = 670

CN_TEXT = {
    'title': 'ESP-IoT-Bridge 网络架构',
    'subtitle': '灵活组合 WAN / LAN 接口，实现多种联网方案',
    'center_title': 'ESP-IoT-Bridge',
    'left_title': '连接互联网',
    'left_sub': 'External Netif · 可选择一个或多个',
    'right_title': '数据转发上网',
    'right_sub': 'Data Forwarding Netif · 可选择一个或多个',
    'wan_side': 'WAN 侧',
    'lan_side': 'LAN 侧',
    'router': '路由器',
    'router_en': 'Router',
    'mcu_host': 'Host',
    'mcu_spi_left': 'SPI/SDIO 直连',
    'mcu_spi_right': 'SPI/SDIO/USB',
    'phone': '手机 /',
    'wifi_dev': 'Wi-Fi 设备',
    'eth_dev': 'ETH 设备',
    'ble_dev': 'BLE 设备',
    'thread_dev': 'Thread 设备',
    'eth_mutex': 'Ethernet 不可同时使用',
    'api_switch': '可调用 API 切换 WAN/LAN',
}

EN_TEXT = {
    'title': 'ESP-IoT-Bridge Architecture',
    'subtitle': 'Flexible WAN / LAN interface combinations for diverse networking scenarios',
    'center_title': 'ESP-IoT-Bridge',
    'left_title': 'Connect to Internet',
    'left_sub': 'External Netif · Select one or more',
    'right_title': 'Data Forwarding',
    'right_sub': 'Data Forwarding Netif · Select one or more',
    'wan_side': 'WAN',
    'lan_side': 'LAN',
    'router': 'Router',
    'router_en': '',
    'mcu_host': 'Host',
    'mcu_spi_left': 'SPI/SDIO direct',
    'mcu_spi_right': 'SPI/SDIO/USB',
    'phone': 'Phone /',
    'wifi_dev': 'Wi-Fi Device',
    'eth_dev': 'ETH Device',
    'ble_dev': 'BLE Device',
    'thread_dev': 'Thread Device',
    'eth_mutex': 'Ethernet: cannot use both sides at once',
    'api_switch': 'Switch WAN/LAN via API',
}


def build_svg(t: dict, lang: str) -> str:
    font = (
        "'Segoe UI', 'PingFang SC', 'Microsoft YaHei', Arial, sans-serif"
        if lang == 'cn'
        else "'Segoe UI', Arial, sans-serif"
    )
    router_sub = (
        f'<text x="93" y="286" text-anchor="middle" font-size="10" fill="#718096">{t["router_en"]}</text>'
        if t['router_en']
        else ''
    )
    eth_mutex_w = '164' if lang == 'cn' else '204'
    eth_mutex_x = '468' if lang == 'cn' else '448'
    api_w = '168' if lang == 'cn' else '178'
    api_x = '466' if lang == 'cn' else '461'

    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="{HEIGHT}" viewBox="0 0 1100 {HEIGHT}" font-family="{font}">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#f7fafc"/><stop offset="100%" stop-color="#edf2f7"/>
    </linearGradient>
    <linearGradient id="espGrad" x1="0%" y1="0%" x2="0%" y2="100%">
      <stop offset="0%" stop-color="#3d4f63"/><stop offset="100%" stop-color="#2b3645"/>
    </linearGradient>
    <linearGradient id="wanGrad" x1="0%" y1="0%" x2="100%" y2="0%">
      <stop offset="0%" stop-color="#1f8a4c"/><stop offset="100%" stop-color="#27ae60"/>
    </linearGradient>
    <linearGradient id="lanGrad" x1="0%" y1="0%" x2="100%" y2="0%">
      <stop offset="0%" stop-color="#e67e22"/><stop offset="100%" stop-color="#f39c12"/>
    </linearGradient>
    <filter id="shadow" x="-10%" y="-10%" width="120%" height="120%">
      <feDropShadow dx="0" dy="3" stdDeviation="6" flood-color="#000000" flood-opacity="0.12"/>
    </filter>
    <marker id="arrowWanTo" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#1f8a4c"/></marker>
    <marker id="arrowLan" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#e67e22"/></marker>
    <marker id="arrowGrayTo" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#607d8b"/></marker>
    <marker id="arrowBlue" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#1e88e5"/></marker>
    <marker id="arrowPurple" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#8e24aa"/></marker>
    <marker id="arrowTealTo" markerWidth="10" markerHeight="10" refX="10" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M0,0 L10,5 L0,10 Z" fill="#00897b"/></marker>
  </defs>
  <rect width="1100" height="{HEIGHT}" fill="url(#bg)"/>
  <text x="550" y="42" text-anchor="middle" font-size="24" font-weight="700" fill="#1a202c">{t["title"]}</text>
  <text x="550" y="68" text-anchor="middle" font-size="13" fill="#718096">{t["subtitle"]}</text>

  <rect x="28" y="92" width="250" height="548" rx="14" fill="#ffffff" stroke="#c6e6d0" stroke-width="2" filter="url(#shadow)"/>
  <rect x="28" y="92" width="250" height="60" rx="14" fill="url(#wanGrad)"/>
  <text x="153" y="122" text-anchor="middle" font-size="15" font-weight="700" fill="#ffffff">{t["left_title"]}</text>
  <text x="153" y="142" text-anchor="middle" font-size="11" fill="#e8f8ef">{t["left_sub"]}</text>

  <rect x="822" y="92" width="250" height="548" rx="14" fill="#ffffff" stroke="#f5d0b3" stroke-width="2" filter="url(#shadow)"/>
  <rect x="822" y="92" width="250" height="60" rx="14" fill="url(#lanGrad)"/>
  <text x="947" y="122" text-anchor="middle" font-size="15" font-weight="700" fill="#ffffff">{t["right_title"]}</text>
  <text x="947" y="142" text-anchor="middle" font-size="11" fill="#fff5eb">{t["right_sub"]}</text>

  <rect x="318" y="120" width="464" height="500" rx="18" fill="url(#espGrad)" filter="url(#shadow)"/>
  <text x="550" y="168" text-anchor="middle" font-size="20" font-weight="700" fill="#ffffff" letter-spacing="0.5">{t["center_title"]}</text>
  <line x1="550" y1="188" x2="550" y2="590" stroke="#4a5568" stroke-width="1.5" stroke-dasharray="6 4"/>
  <text x="420" y="206" text-anchor="middle" font-size="11" fill="#90cdf4" font-weight="600">{t["wan_side"]}</text>
  <text x="680" y="206" text-anchor="middle" font-size="11" fill="#fbd38d" font-weight="600">{t["lan_side"]}</text>

  <rect x="340" y="236" width="88" height="34" rx="8" fill="#e8f5ec" stroke="#1f8a4c" stroke-width="1.5"/><text x="384" y="258" text-anchor="middle" font-size="12" font-weight="600" fill="#1f8a4c">Station</text>
  <rect x="340" y="286" width="88" height="34" rx="8" fill="#e8f5ec" stroke="#1f8a4c" stroke-width="1.5" stroke-dasharray="5 3"/><text x="384" y="308" text-anchor="middle" font-size="12" font-weight="600" fill="#1f8a4c">Ethernet</text>
  <rect x="340" y="336" width="88" height="34" rx="8" fill="#e0f2f1" stroke="#00897b" stroke-width="1.5"/><text x="384" y="358" text-anchor="middle" font-size="12" font-weight="600" fill="#00897b">4G UART</text>
  <rect x="340" y="386" width="88" height="34" rx="8" fill="#e0f2f1" stroke="#00897b" stroke-width="1.5"/><text x="384" y="408" text-anchor="middle" font-size="12" font-weight="600" fill="#00897b">4G USB</text>
  <rect x="340" y="446" width="88" height="34" rx="8" fill="#e8f5ec" stroke="#1f8a4c" stroke-width="1.5" stroke-dasharray="5 3"/><text x="384" y="468" text-anchor="middle" font-size="12" font-weight="600" fill="#1f8a4c">SPI</text>
  <rect x="340" y="496" width="88" height="34" rx="8" fill="#e8f5ec" stroke="#1f8a4c" stroke-width="1.5" stroke-dasharray="5 3"/><text x="384" y="518" text-anchor="middle" font-size="12" font-weight="600" fill="#1f8a4c">SDIO</text>

  <rect x="672" y="236" width="88" height="34" rx="8" fill="#fef3e8" stroke="#e67e22" stroke-width="1.5"/><text x="716" y="258" text-anchor="middle" font-size="12" font-weight="600" fill="#e67e22">SoftAP</text>
  <rect x="672" y="286" width="88" height="34" rx="8" fill="#fef3e8" stroke="#e67e22" stroke-width="1.5" stroke-dasharray="5 3"/><text x="716" y="308" text-anchor="middle" font-size="12" font-weight="600" fill="#e67e22">Ethernet</text>
  <rect x="672" y="366" width="88" height="34" rx="8" fill="#fef3e8" stroke="#e67e22" stroke-width="1.5"/><text x="716" y="388" text-anchor="middle" font-size="12" font-weight="600" fill="#e67e22">USB</text>
  <rect x="672" y="416" width="88" height="34" rx="8" fill="#fef3e8" stroke="#e67e22" stroke-width="1.5" stroke-dasharray="5 3"/><text x="716" y="438" text-anchor="middle" font-size="12" font-weight="600" fill="#e67e22">SPI</text>
  <rect x="672" y="466" width="88" height="34" rx="8" fill="#fef3e8" stroke="#e67e22" stroke-width="1.5" stroke-dasharray="5 3"/><text x="716" y="488" text-anchor="middle" font-size="12" font-weight="600" fill="#e67e22">SDIO</text>
  <rect x="672" y="526" width="88" height="34" rx="8" fill="#e3f2fd" stroke="#1e88e5" stroke-width="1.5"/><text x="716" y="548" text-anchor="middle" font-size="12" font-weight="600" fill="#1e88e5">BLE</text>
  <rect x="672" y="576" width="88" height="34" rx="8" fill="#f3e5f5" stroke="#8e24aa" stroke-width="1.5"/><text x="716" y="592" text-anchor="middle" font-size="11" font-weight="600" fill="#8e24aa">Thread</text><text x="716" y="604" text-anchor="middle" font-size="9" fill="#8e24aa">Router</text>

  <rect x="48" y="230" width="90" height="86" rx="10" fill="#ffffff" stroke="#1f8a4c" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="93" y="271" text-anchor="middle" font-size="12" font-weight="600" fill="#2d3748">{t["router"]}</text>{router_sub}
  <rect x="48" y="334" width="90" height="86" rx="10" fill="#ffffff" stroke="#00897b" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="93" y="382" text-anchor="middle" font-size="13" font-weight="600" fill="#00897b">4G</text>
  <rect x="48" y="440" width="90" height="86" rx="10" fill="#eceff1" stroke="#607d8b" stroke-width="2" filter="url(#shadow)"/>
  <text x="93" y="474" text-anchor="middle" font-size="14" font-weight="700" fill="#455a64">MCU</text>
  <text x="93" y="492" text-anchor="middle" font-size="10" fill="#78909c">{t["mcu_host"]}</text>
  <text x="93" y="508" text-anchor="middle" font-size="9" fill="#e65100">{t["mcu_spi_left"]}</text>

  <rect x="962" y="228" width="90" height="45" rx="10" fill="#ffffff" stroke="#e67e22" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="1007" y="247" text-anchor="middle" font-size="11" font-weight="600" fill="#2d3748">{t["phone"]}</text>
  <text x="1007" y="263" text-anchor="middle" font-size="11" font-weight="600" fill="#2d3748">{t["wifi_dev"]}</text>
  <rect x="962" y="283" width="90" height="42" rx="10" fill="#ffffff" stroke="#e67e22" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="1007" y="308" text-anchor="middle" font-size="12" font-weight="600" fill="#2d3748">{t["eth_dev"]}</text>
  <rect x="962" y="375" width="90" height="116" rx="10" fill="#eceff1" stroke="#607d8b" stroke-width="2" filter="url(#shadow)"/>
  <text x="1007" y="415" text-anchor="middle" font-size="14" font-weight="700" fill="#455a64">MCU</text>
  <text x="1007" y="435" text-anchor="middle" font-size="10" fill="#78909c">{t["mcu_host"]}</text>
  <text x="1007" y="455" text-anchor="middle" font-size="9" fill="#e65100">{t["mcu_spi_right"]}</text>
  <rect x="962" y="521" width="90" height="44" rx="10" fill="#ffffff" stroke="#1e88e5" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="1007" y="548" text-anchor="middle" font-size="11" font-weight="600" fill="#1e88e5">{t["ble_dev"]}</text>
  <rect x="962" y="571" width="90" height="44" rx="10" fill="#ffffff" stroke="#8e24aa" stroke-width="1.5" filter="url(#shadow)"/>
  <text x="1007" y="598" text-anchor="middle" font-size="11" font-weight="600" fill="#8e24aa">{t["thread_dev"]}</text>

  <line x1="340" y1="253" x2="138" y2="253" stroke="#1f8a4c" stroke-width="2" marker-end="url(#arrowWanTo)"/>
  <line x1="340" y1="303" x2="138" y2="303" stroke="#1f8a4c" stroke-width="2" marker-end="url(#arrowWanTo)"/>
  <line x1="340" y1="353" x2="138" y2="353" stroke="#00897b" stroke-width="2" marker-end="url(#arrowTealTo)"/>
  <line x1="340" y1="403" x2="138" y2="403" stroke="#00897b" stroke-width="2" marker-end="url(#arrowTealTo)"/>
  <line x1="340" y1="463" x2="138" y2="463" stroke="#607d8b" stroke-width="2" marker-end="url(#arrowGrayTo)"/>
  <line x1="340" y1="513" x2="138" y2="513" stroke="#607d8b" stroke-width="2" marker-end="url(#arrowGrayTo)"/>

  <line x1="760" y1="253" x2="962" y2="253" stroke="#e67e22" stroke-width="2" marker-end="url(#arrowLan)"/>
  <line x1="760" y1="303" x2="962" y2="303" stroke="#e67e22" stroke-width="2" marker-end="url(#arrowLan)"/>
  <line x1="760" y1="383" x2="962" y2="383" stroke="#e67e22" stroke-width="2" marker-end="url(#arrowLan)"/>
  <line x1="760" y1="433" x2="962" y2="433" stroke="#e67e22" stroke-width="2" marker-end="url(#arrowLan)"/>
  <line x1="760" y1="483" x2="962" y2="483" stroke="#e67e22" stroke-width="2" marker-end="url(#arrowLan)"/>
  <line x1="760" y1="543" x2="962" y2="543" stroke="#1e88e5" stroke-width="2" marker-end="url(#arrowBlue)"/>
  <line x1="760" y1="593" x2="962" y2="593" stroke="#8e24aa" stroke-width="2" marker-end="url(#arrowPurple)"/>

  <path d="M 428 303 Q 550 340 672 303" fill="none" stroke="#c0392b" stroke-width="2" stroke-dasharray="7 4"/>
  <rect x="{eth_mutex_x}" y="318" width="{eth_mutex_w}" height="22" rx="6" fill="#fff5f5" stroke="#e53e3e" stroke-width="1"/>
  <text x="550" y="333" text-anchor="middle" font-size="10" font-weight="600" fill="#c53030">{t["eth_mutex"]}</text>
  <path d="M 428 463 Q 550 440 672 433" fill="none" stroke="#c0392b" stroke-width="1.8" stroke-dasharray="7 4"/>
  <path d="M 428 513 Q 550 490 672 483" fill="none" stroke="#c0392b" stroke-width="1.8" stroke-dasharray="7 4"/>
  <rect x="{api_x}" y="455" width="{api_w}" height="20" rx="5" fill="#fffaf0" stroke="#dd6b20" stroke-width="1"/>
  <text x="550" y="469" text-anchor="middle" font-size="10" font-weight="600" fill="#c05621">{t["api_switch"]}</text>
</svg>'''


def main() -> None:
    import subprocess

    pairs = [
        (CN_TEXT, 'cn', 'esp_iot_bridge.svg', 'esp_iot_bridge.png'),
        (EN_TEXT, 'en', 'esp_iot_bridge_en.svg', 'esp_iot_bridge_en.png'),
    ]
    for text, lang, svg_name, png_name in pairs:
        svg_path = OUT_DIR / svg_name
        png_path = OUT_DIR / png_name
        svg_path.write_text(build_svg(text, lang), encoding='utf-8')
        subprocess.run(
            [
                'google-chrome',
                '--headless',
                '--disable-gpu',
                f'--window-size=1100,{HEIGHT}',
                f'--screenshot={png_path}',
                f'file://{svg_path.resolve()}',
            ],
            check=True,
            capture_output=True,
        )
        print(f'Generated {svg_path.name} and {png_path.name}')


if __name__ == '__main__':
    main()
