# MicroPython para Raspberry Pi Pico W
# Reescrita do controle de ar-condicionado com DS18B20 + servidor HTTP
#Código teste gerado por IA

import network
import socket
import machine
import time
import onewire
import ds18x20

# ============ CONFIGURAÇÕES ============
SSID = "SEU_SSID"
PASSWORD = "SENHA"

GPIO_COMPRESSOR = 23          # relé do compressor (ativo em LOW)
GPIO_VENT_BAIXO = 18          # relé/saída ventilador baixo
GPIO_VENT_ALTO  = 19          # relé/saída ventilador alto
GPIO_TEMP_DATA  = 21          # DS18B20 DATA (com pull-up 4k7 para 3V3)

INTERVALO_COMPRESSOR_MS = 90_000   # 90 s anti-curto-ciclo

TEMP_MIN = 15.0
TEMP_MAX = 35.0

# ============ GPIO ============
pin_comp = machine.Pin(GPIO_COMPRESSOR, machine.Pin.OUT, value=1)  # desligado (alto)
pin_vb   = machine.Pin(GPIO_VENT_BAIXO, machine.Pin.OUT, value=1)  # desligado (alto)
pin_va   = machine.Pin(GPIO_VENT_ALTO,  machine.Pin.OUT, value=1)  # desligado (alto)

# ============ DS18B20 ============
ow = onewire.OneWire(machine.Pin(GPIO_TEMP_DATA))
ds = ds18x20.DS18X20(ow)
roms = ds.scan()
if not roms:
        raise RuntimeError("Nenhum DS18B20 encontrado no barramento OneWire.")
ROM0 = roms[0]

def ler_temperatura():
        ds.convert_temp()
        time.sleep_ms(750)
        t = ds.read_temp(ROM0)
        # Alguns firmwares retornam None ocasionalmente; trate isso:
        return t if t is not None else float('nan')

# ============ ESTADO ============
temperatura_desejada = 20.0
potencia_vent = 0                 # 0=desligado, 1=baixa, 2=alta
ar_ligado = False
compressor_ligado = False
ultima_troca_ms = time.ticks_ms()

def pode_trocar(agora_ms):
        return time.ticks_diff(agora_ms, ultima_troca_ms) > INTERVALO_COMPRESSOR_MS

def ligar_compressor():
        global compressor_ligado, ultima_troca_ms
        pin_comp.value(0)  # ativo em LOW
        compressor_ligado = True
        ultima_troca_ms = time.ticks_ms()

def desligar_compressor():
        global compressor_ligado, ultima_troca_ms
        pin_comp.value(1)  # inativo em HIGH
        compressor_ligado = False
        ultima_troca_ms = time.ticks_ms()

def ventilador_desligado():
        pin_vb.value(1)
        pin_va.value(1)

def ventilador_baixo():
        pin_vb.value(0)
        pin_va.value(1)

def ventilador_alto():
        pin_vb.value(1)
        pin_va.value(0)

def aplicar_potencia_vent():
        if potencia_vent == 2:
                ventilador_alto()
        elif potencia_vent == 1:
                ventilador_baixo()
        else:
                ventilador_desligado()

def aumentar_temp():
        global temperatura_desejada
        if temperatura_desejada < TEMP_MAX:
                temperatura_desejada += 1.0

def diminuir_temp():
        global temperatura_desejada
        if temperatura_desejada > TEMP_MIN:
                temperatura_desejada -= 1.0

def refrigerar(agora_ms):
        if not compressor_ligado and pode_trocar(agora_ms):
                ligar_compressor()

def desligar_ar(agora_ms):
        if compressor_ligado and pode_trocar(agora_ms):
                desligar_compressor()
        ventilador_desligado()

# ============ WIFI ============
def conectar_wifi():
        wlan = network.WLAN(network.STA_IF)
        wlan.active(True)
        if not wlan.isconnected():
                wlan.connect(SSID, PASSWORD)
                # espera com timeout simples
                t0 = time.ticks_ms()
                while not wlan.isconnected() and time.ticks_diff(time.ticks_ms(), t0) < 15000:
                        time.sleep_ms(200)
        if not wlan.isconnected():
                raise RuntimeError("Falha ao conectar no Wi-Fi.")
        return wlan

wlan = conectar_wifi()
ip = wlan.ifconfig()[0]

# ============ HTTP ============
HTML_BASE = """\
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Connection: close

<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Controle Ar-Condicionado (Pico W)</title>
<style>
body{margin:0;font-family:Arial,Helvetica,sans-serif;background:#000;color:#fff}
.wrap{max-width:720px;margin:0 auto;padding:16px}
h1{font-size:22px;margin:0 0 12px}
.card{background:#111;border:1px solid #222;border-radius:10px;padding:16px;margin:12px 0}
.btn{display:inline-block;background:#444;color:#fff;text-decoration:none;padding:10px 14px;border-radius:8px;margin:4px 6px}
.row{display:flex;gap:8px;flex-wrap:wrap}
.badge{display:inline-block;padding:4px 8px;border-radius:6px;background:#222;border:1px solid #333;margin-left:8px}
.big{font-size:48px;font-weight:700}
.small{opacity:.8}
</style>
</head>
<body>
<div class="wrap">
  <h1>Controle Ar-Condicionado<span class="badge">{estado}</span></h1>
  <div class="card">
    <div>Temperatura desejada</div>
    <div class="big">{temp_set}°C</div>
    <div class="small">Ambiente: {temp_amb}°C | Compressor: {comp}</div>
  </div>
  <div class="card">
    <div class="row">
      <a class="btn" href="/ligar-desligar">{botao_power}</a>
      <a class="btn" href="/mais">+1°C</a>
      <a class="btn" href="/menos">-1°C</a>
      <a class="btn" href="/v2">Vent. Alta</a>
      <a class="btn" href="/v1">Vent. Baixa</a>
      <a class="btn" href="/v0">Vent. Desligado</a>
      <a class="btn" href="/info">Info</a>
    </div>
  </div>
</div>
</body>
</html>
"""

def resposta_http(amb, estado, comp_on, botao):
        s_amb = "--" if amb is None or amb != amb else f"{amb:.1f}"
        s_comp = "Ligado" if comp_on else "Desligado"
        return HTML_BASE.format(
                estado=("Ligado" if estado else "Desligado"),
                temp_set=f"{temperatura_desejada:.0f}" if estado else "--",
                temp_amb=s_amb,
                comp=s_comp,
                botao_power=("Desligar" if estado else "Ligar")
        )

def iniciar_servidor():
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(("", 80))
        s.listen(1)
        s.settimeout(0.1)
        return s

srv = iniciar_servidor()

# ============ LOOP PRINCIPAL ============
temp_cache = None
temp_last_ms = 0

print("IP:", ip)

while True:
        agora = time.ticks_ms()

        if time.ticks_diff(agora, temp_last_ms) > 1500:
                temp_cache = ler_temperatura()
                temp_last_ms = agora

        if ar_ligado:
                if temp_cache == temp_cache and temp_cache is not None and temp_cache > temperatura_desejada:
                        refrigerar(agora)
                else:
                        if compressor_ligado and pode_trocar(agora):
                                desligar_compressor()
                aplicar_potencia_vent()
        else:
                desligar_ar(agora)

        try:
                cl, addr = srv.accept()
        except OSError:
                cl = None

        if cl:
                cl.settimeout(0.5)
                try:
                        req = cl.recv(1024)
                        line = req.split(b"\r\n", 1)[0] if req else b""
                        # parsing básico do método + caminho
                        parts = line.split()
                        caminho = parts[1].decode() if len(parts) >= 2 else "/"

                        if caminho.startswith("/ligar-desligar"):
                                ar_ligado = not ar_ligado
                        elif caminho.startswith("/mais"):
                                aumentar_temp()
                        elif caminho.startswith("/menos"):
                                diminuir_temp()
                        elif caminho.startswith("/v2"):
                                potencia_vent = 2
                        elif caminho.startswith("/v1"):
                                potencia_vent = 1
                        elif caminho.startswith("/v0"):
                                potencia_vent = 0
                        elif caminho.startswith("/info"):
                                print("Temperatura Desejada:", temperatura_desejada, "°C",
                                      "| Ambiente:", temp_cache, "°C")
                                print("Compressor:", "Ligado" if compressor_ligado else "Desligado")
                                print("ms desde boot:", agora)
                                print()

                        html = resposta_http(
                                amb=temp_cache,
                                estado=ar_ligado,
                                comp_on=compressor_ligado,
                                botao=("Desligar" if ar_ligado else "Ligar")
                        )
                        cl.sendall(html.encode())
                except Exception as e:
                        try:
                                cl.close()
                        except:
                                pass
                finally:
                        try:
                                cl.close()
                        except:
                                pass

        time.sleep_ms(100)