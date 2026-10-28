import json
import math
import os
import struct
import subprocess
import wave
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
MEDIA = ROOT / "examples" / "media" / "stage16"
ART = ROOT / "artifacts" / "antes-do-primeiro-gole-stage16"
MEDIA.mkdir(parents=True, exist_ok=True)
ART.mkdir(parents=True, exist_ok=True)

def svg(name, body, width=1280, height=720):
    content = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<defs><linearGradient id="bg" x2="0" y2="1"><stop stop-color="#07151c"/><stop offset="1" stop-color="#102b32"/></linearGradient><linearGradient id="water" x2="1" y2="1"><stop stop-color="#58d4c5"/><stop offset="1" stop-color="#5b9ec2"/></linearGradient><filter id="soft"><feGaussianBlur stdDeviation="12"/></filter></defs>{body}</svg>'''
    (MEDIA / name).write_text(content, encoding="utf-8")

svg("watershed.svg", '''<rect width="1280" height="720" fill="url(#bg)"/><path d="M0 420Q180 250 350 390T710 345T1050 380T1280 300V720H0Z" fill="#183a3c"/><path d="M0 490Q180 365 350 475T710 420T1050 468T1280 390" fill="none" stroke="url(#water)" stroke-width="42" opacity=".68"/><path d="M-30 570Q210 450 450 555T890 505T1320 480" fill="none" stroke="#b1efe5" stroke-width="2" opacity=".55"/><circle cx="260" cy="390" r="7" fill="#f2cc81"/><circle cx="950" cy="420" r="7" fill="#f2cc81"/>''')
svg("micro-world.svg", '''<rect width="1280" height="720" fill="#090f1c"/><circle cx="640" cy="360" r="250" fill="#102942" stroke="#58d4c5" stroke-width="3" opacity=".9"/><circle cx="640" cy="360" r="218" fill="none" stroke="#4c8090" stroke-width="1" opacity=".6"/><g fill="#f0bb6c" opacity=".88"><circle cx="510" cy="300" r="8"/><circle cx="690" cy="260" r="5"/><circle cx="770" cy="430" r="7"/><circle cx="560" cy="470" r="4"/><circle cx="630" cy="375" r="3"/></g><g fill="none" stroke="#65d3c7" stroke-width="3" opacity=".8"><path d="M510 300l84-68 96 28 80 92-18 78-112 68-80-28-55-98z"/><path d="M594 232l36 143 60-115m-60 115 112 68"/></g><text x="640" y="650" text-anchor="middle" fill="#9bc0c6" font-family="Segoe UI" font-size="18" letter-spacing="4">VISUALIZAÇÃO ILUSTRATIVA · NÃO É UMA AMOSTRA</text>''')
svg("treatment.svg", '''<rect width="1280" height="720" fill="url(#bg)"/><path d="M80 470H1200" stroke="#42727a" stroke-width="3"/><g fill="#142d35" stroke="#58cbbb" stroke-width="2"><path d="M100 300h190l-18 180H118z"/><path d="M330 300h190l-18 180H348z"/><path d="M560 300h190l-18 180H578z"/><path d="M790 300h190l-18 180H808z"/><path d="M1020 300h170l-18 180h-134z"/></g><g fill="#d9b56b" opacity=".86"><circle cx="150" cy="380" r="6"/><circle cx="202" cy="410" r="4"/><circle cx="375" cy="390" r="8"/><circle cx="420" cy="430" r="6"/><circle cx="635" cy="430" r="4"/><circle cx="830" cy="400" r="3"/></g><path d="M95 500H1190" stroke="url(#water)" stroke-width="10"/><g fill="#edf5ec" font-family="Segoe UI" font-size="17" text-anchor="middle"><text x="195" y="545">AGREGAR</text><text x="425" y="545">AGRUPAR</text><text x="655" y="545">SEPARAR</text><text x="885" y="545">FILTRAR</text><text x="1100" y="545">DESINFETAR</text></g><text x="640" y="640" text-anchor="middle" fill="#a9c6c8" font-family="Segoe UI" font-size="16">ESQUEMA DIDÁTICO · A SEQUÊNCIA VARIA CONFORME A ÁGUA DE ORIGEM</text>''')
svg("city-network.svg", '''<rect width="1280" height="720" fill="#08141d"/><path d="M0 590H1280" stroke="#29424b" stroke-width="3"/><g fill="#112732" stroke="#355661" stroke-width="2"><path d="M100 330h140v260H100zM280 260h190v330H280zM520 365h150v225H520zM720 220h190v370H720zM950 310h130v280H950zM1110 390h110v200h-110z"/></g><g fill="#66ccbd" opacity=".55"><path d="M130 370h22v30h-22zm56 0h22v30h-22zM320 300h25v33h-25zm85 0h25v33h-25zM760 270h25v34h-25zm80 0h25v34h-25zM980 350h22v30h-22z"/></g><path d="M0 620C200 540 320 670 510 610S800 560 980 620s210 40 300-5" fill="none" stroke="#50c4bf" stroke-width="12" opacity=".8"/><circle cx="180" cy="610" r="10" fill="#f0c378"/><circle cx="760" cy="602" r="10" fill="#f0c378"/><circle cx="1120" cy="600" r="10" fill="#f0c378"/>''')
svg("watershed-travel.svg", '''<rect width="2600" height="720" fill="url(#bg)"/><path d="M0 420Q180 250 350 390T710 345T1050 380T1400 300T1750 380T2100 320T2600 370V720H0Z" fill="#183a3c"/><path d="M0 490Q180 365 350 475T710 420T1050 468T1400 390T1750 470T2100 410T2600 465" fill="none" stroke="url(#water)" stroke-width="42" opacity=".72"/><path d="M-30 570Q210 450 450 555T890 505T1320 480T1760 550T2180 490T2640 520" fill="none" stroke="#b1efe5" stroke-width="2" opacity=".55"/><circle cx="260" cy="390" r="7" fill="#f2cc81"/><circle cx="950" cy="420" r="7" fill="#f2cc81"/><circle cx="1450" cy="414" r="7" fill="#f2cc81"/><circle cx="2250" cy="425" r="7" fill="#f2cc81"/>''', 2600, 720)
svg("city-network-wide.svg", '''<rect width="2600" height="720" fill="#08141d"/><path d="M0 590H2600" stroke="#29424b" stroke-width="3"/><g fill="#112732" stroke="#355661" stroke-width="2"><path d="M80 360h140v230H80zM260 290h170v300H260zM470 380h150v210H470zM660 225h190v365H660zM900 330h140v260H900zM1100 270h210v320h-210zM1370 355h160v235h-160zM1580 220h210v370h-210zM1840 300h150v290h-150zM2040 370h180v220h-180zM2270 250h210v340h-210z"/></g><g fill="#66ccbd" opacity=".55"><path d="M115 395h22v30h-22zm58 0h22v30h-22zM300 330h25v32h-25zm78 0h25v32h-25zM700 270h25v32h-25zm80 0h25v32h-25zM1150 310h25v32h-25zm80 0h25v32h-25zM1620 260h25v32h-25zm90 0h25v32h-25zM2300 295h25v32h-25zm88 0h25v32h-25z"/></g><path d="M0 620C210 540 340 670 550 610S900 560 1110 620s340 40 520-8 370 20 520-12 300 15 450-12" fill="none" stroke="#50c4bf" stroke-width="12" opacity=".82"/><circle cx="180" cy="610" r="10" fill="#f0c378"/><circle cx="760" cy="602" r="10" fill="#f0c378"/><circle cx="1420" cy="602" r="10" fill="#f0c378"/><circle cx="2180" cy="600" r="10" fill="#f0c378"/>''', 2600, 720)

# A locally generated screen illustration and an original audio/video bed. Both are marked as simulations.
im = Image.new("RGB", (480, 900), "#0b1d2a")
d = ImageDraw.Draw(im)
d.rectangle((24, 24, 456, 876), outline="#4cc8b5", width=3)
d.text((48, 60), "PAINEL ILUSTRATIVO", fill="#a8c9c7")
d.text((48, 110), "SIMULAÇÃO VISUAL", fill="#f1c778")
d.text((48, 170), "Sem amostra real", fill="#edf4f2")
d.line((48, 250, 430, 250), fill="#315b67", width=2)
for i, h in enumerate([76, 112, 88, 155, 132, 210, 175, 238]):
    x = 54 + i * 45
    d.rounded_rectangle((x, 690 - h, x + 22, 690), radius=8, fill="#54cbb9")
d.text((48, 735), "DADOS FICTÍCIOS", fill="#a8c9c7")
im.save(MEDIA / "monitor-simulacao.png")

def write_wav(path, seconds, base, sweep=0):
    rate = 24000
    with wave.open(str(path), "wb") as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(rate)
        data = bytearray()
        for i in range(int(rate * seconds)):
            t = i / rate
            env = min(1, t * 3, (seconds - t) * 2)
            freq = base + sweep * (t / seconds)
            v = int(32767 * .13 * env * math.sin(2 * math.pi * freq * t))
            data += struct.pack("<h", v)
        f.writeframes(data)

def write_ambient(path, seconds=30):
    rate = 24000
    with wave.open(str(path), "wb") as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(rate)
        data = bytearray()
        for i in range(int(rate * seconds)):
            t = i / rate
            env = min(1, t * 2, (seconds - t) * 2)
            slow = 0.7 + 0.3 * math.sin(2 * math.pi * 0.035 * t)
            low = math.sin(2 * math.pi * 73 * t) * 0.035
            fifth = math.sin(2 * math.pi * 109.27 * t + 0.12 * math.sin(2 * math.pi * .08 * t)) * 0.018
            high = math.sin(2 * math.pi * (247 + 2 * math.sin(2 * math.pi * .05 * t)) * t) * 0.006
            value = int(32767 * env * slow * (low + fifth + high))
            data += struct.pack("<h", value)
        f.writeframes(data)

write_ambient(MEDIA / "ambience-original.wav")
write_wav(MEDIA / "drop-original.wav", 1.5, 520, -260)
write_wav(MEDIA / "transition-original.wav", 2.0, 190, 220)

ffmpeg = Path(ROOT / ".tools" / "python" / "imageio_ffmpeg" / "binaries")
enc = next(ffmpeg.glob("ffmpeg*.exe"))
raw = subprocess.Popen([str(enc), "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "640x360", "-r", "24", "-i", "-", "-an", "-c:v", "libx264", "-preset", "ultrafast", "-crf", "23", "-pix_fmt", "yuv420p", str(MEDIA / "water-current-original.mp4")], stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
for frame in range(96):
    t = frame / 24
    pic = Image.new("RGB", (640, 360), "#071822")
    dr = ImageDraw.Draw(pic)
    for j in range(6):
        y = 110 + j * 32 + int(9 * math.sin(t * 2 + j))
        dr.arc((-180 + j * 44 + int(20 * math.sin(t)), y - 90, 760 + j * 20, y + 100), 190, 345, fill=(40 + j * 10, 132 + j * 9, 150 + j * 8), width=4)
    for j in range(22):
        x = (j * 53 + int(t * 75)) % 660
        y = 28 + (j * 73) % 300
        dr.ellipse((x, y, x + 2, y + 2), fill="#99e3dc")
    raw.stdin.write(pic.tobytes())
raw.stdin.close()
if raw.wait() != 0:
    raise SystemExit("Falha ao gerar o clipe original de água.")

def t(kind, ident, text, x, y, w, h, size=32, color="#ECF4F1", **extra):
    return {"id": ident, "kind": kind, "x": x, "y": y, "width": w, "height": h, "text": text,
            "fontSize": size, "color": color, "motion": "none", **extra}

def shape(ident, x, y, w, h, color, **extra):
    return {"id": ident, "kind": "shape", "x": x, "y": y, "width": w, "height": h,
            "color": color, "opacity": 1, "motion": "none", **extra}

def native(ident, asset, x, y, w, h, color="#5CC9BC", **extra):
    return {"id": ident, "kind": "native", "asset": asset, "x": x, "y": y, "width": w,
            "height": h, "color": color, "motion": "none", **extra}

def media(ident, kind, source, x, y, w, h, **extra):
    return {"id": ident, "kind": kind, "source": source, "x": x, "y": y, "width": w,
            "height": h, "motion": "none", **extra}

def prem(ident, asset, slots, palette, cue, opacity=1):
    return {"id": ident, "kind": "library", "assetId": "premium:" + asset,
            "cue": cue, "slots": slots, "palette": palette, "motion": "none", "opacity": opacity}

def shot(ident, cue, els, transition="cut", camera=None, beats=None, name=None):
    return {"id": ident, "name": name or ident.replace("-", " ").title(),
            "start": {"cue": cue, "cueProgress": 0},
            "transitionIn": ("cut" if transition == "cut" else {"type": transition, "durationFrames": 12}),
            "camera": camera or {"x": 640, "y": 360, "zoom": 1, "rotation": 0},
            "elements": els, **({"beats": beats} if beats else {})}

def ambience(scene):
    return [{"id": scene + "-ambience", "name": "Ambiência sintética original", "source": "media/stage16/ambience-original.wav",
             "start": {"cue": 0, "cueProgress": 0}, "gain": 0.13, "duckUnderVoice": True, "duckAmount": 0.78,
             "fadeInFrames": 36, "fadeOutFrames": 90}]

scenes = []
def add_scene(id_, cues, shots, title):
    # The first shot in each scene has no outgoing visual predecessor.
    shots[0]["transitionIn"] = "cut"
    scenes.append({"id": id_, "title": title, "narration": cues, "shots": shots, "audioTracks": ambience(id_)})

add_scene("misterio", [
    "A água parece simples. Uma torneira, um copo, um gole.",
    "Mas antes de chegar até nós, ela atravessa uma jornada quase invisível.",
    "E a primeira pergunta é esta: de onde vem a água que você bebe?"], [
    shot("misterio-gota", 0, [shape("fundo-escuro-a", 0, 0, 1280, 720, "#07141C"), shape("gota-sinal", 618, 270, 44, 64, "#55D4C5", opacity=0),
         t("text", "abertura-micro", "ANTES DE CHEGAR AO COPO", 90, 595, 1100, 46, 19, "#8EAEB2", motion="fade"),
         prem("pergunta-agua", "question-hook-01", {"question": "DE ONDE VEM\nA ÁGUA QUE VOCÊ BEBE?", "context": "A jornada invisível da água potável"},
              {"accent":"#55D4C5","highlight":"#E4BB71","muted":"#94B8BC","text":"#EDF5F0"}, 0)],
         "fade", {"x":640,"y":360,"zoom":1.18,"rotation":0}, [
             {"id":"gota-revela","at":{"cue":0,"cueProgress":0.38},"action":"show","target":"gota-sinal"},
             {"id":"gota-pulsa","at":{"cue":0,"cueProgress":0.58},"action":"transform","target":"gota-sinal","durationFrames":18,"scale":1.25},
             {"id":"som-gota","at":{"cue":0,"cueProgress":0.4},"action":"sound","source":"media/stage16/drop-original.wav","gain":0.24}], "A pergunta que abre a jornada"),
    shot("misterio-revela", 1, [shape("fundo-escuro-b", 0, 0, 1280, 720, "#07141C"), media("gota-svg", "svg", "media/stage16/watershed.svg", 0, 0, 1280, 720),
         t("text", "linha-historia", "UMA JORNADA ANTES\nDO PRIMEIRO GOLE", 95, 75, 800, 145, 48, "#F1F5EF", motion="rise"),
         native("ponto-captacao", "map-pin", 924, 337, 106, 150, "#E4BB71", motion="pop")],
         "dissolve", {"x":640,"y":360,"zoom":1,"rotation":0}, None, "O percurso começa fora de quadro"),
    shot("misterio-respiro", 2, [shape("fundo-escuro-c", 0, 0, 1280, 720, "#07141C"),
         prem("titulo-original", "title-reveal-01", {"main-title":"ANTES DO PRIMEIRO GOLE","subtitle":"A jornada invisível da água potável","editorial-id":"DOCUMENTÁRIO · ÁGUA"},
              {"accent":"#55D4C5","secondary":"#87A8AD","highlight":"#E4BB71","text":"#F2F5EE","muted":"#9BB6BA"}, 2)], "cut", None, None, "O título aparece como promessa"),
], "O mistério da torneira")

add_scene("origem", [
    "A história começa longe da torneira, em rios, reservatórios e outras fontes.",
    "Cada origem tem características próprias, e os sistemas precisam conhecer a água que recebem.",
    "Um percurso de muitos lugares começa com um ponto de captação."], [
    shot("origem-mapa", 0, [shape("origem-fundo", 0, 0, 1280, 720, "#0A1C24"), media("mapa-bacia", "svg", "media/stage16/watershed.svg", 0, 0, 1700, 720),
         native("globo-origem", "globe", 165, 190, 140, 140, "#58CBBE"), native("pin-origem", "map-pin", 1110, 330, 80, 120, "#E8BC72"),
         t("text","origem-kicker","01 / FONTE",82,70,360,36,18,"#93C5C1"),
         t("text","origem-titulo","A ÁGUA COMEÇA\nEM OUTRO LUGAR",82,112,600,128,45,"#F1F5EF")], "slide", {"x":220,"y":360,"zoom":1,"rotation":0}, None, "Uma bacia ilustrada"),
    shot("origem-travelling", 1, [shape("origem-travel-fundo",0,0,1280,720,"#0A1C24"),
         {"id":"rio-longo","kind":"svg","source":"media/stage16/watershed-travel.svg","x":0,"y":0,"width":2600,"height":720,"motion":"none"},
         native("origem-alvo-a","map-pin",180,290,80,120,"#E8BC72"), native("origem-alvo-b","factory",1320,255,180,200,"#58CBBE"),
         t("text","origem-label-a","CAPTAÇÃO",95,465,300,52,24,"#EAF4F0"),
         t("text","origem-label-b","SISTEMA DE ABASTECIMENTO",1260,500,560,40,21,"#EAF4F0")], "dissolve",
         {"x":240,"y":360,"zoom":1,"rotation":0,"path":{"safeMargin":0.07,"captionSafeBottom":0.15,"targets":[
             {"target":"origem-alvo-a","at":{"cue":1,"cueProgress":0},"padding":3.0,"reveal":"spatial"},
             {"target":"origem-alvo-b","at":{"cue":1,"cueProgress":0.84},"padding":3.2,"easing":"ease-in-out","reveal":"spatial"}]}}, None, "Travelling espacial da captação ao sistema"),
    shot("origem-ressalva", 2, [shape("origem-fundo-final",0,0,1280,720,"#0A1C24"), native("laboratorio-origem","laboratory-scene",170,210,330,240,"#4FC4B5"),
         prem("origem-fonte", "source-label-01", {"title":"ORIGEM DA ÁGUA","detail":"Ilustração editorial · sem local específico"},
              {"surface":"#112832","highlight":"#E3BA70","text":"#EFF4EF","muted":"#9FB7B8"}, 2)], "cut", None, None, "Sem atribuir uma cidade real"),
], "A origem muda de lugar para lugar")

add_scene("invisivel", [
    "Água transparente não é, por si só, garantia de segurança.",
    "Microrganismos e substâncias podem estar presentes sem serem vistos a olho nu.",
    "A aparência e a potabilidade respondem a perguntas diferentes."], [
    shot("invisivel-copo", 0, [shape("invisivel-fundo-a",0,0,1280,720,"#09141D"), native("lupa-agua","magnifier",885,205,180,180,"#59CDBD"),
         native("frasco-agua","flask",420,250,170,205,"#E3BD74"),
         t("text","transparente","TRANSPARENTE\nNÃO É SINÔNIMO\nDE POTÁVEL",104,206,630,220,42,"#F0F5F1"),
         t("text","visual-only","REPRESENTAÇÃO CONCEITUAL",107,548,520,32,16,"#94B8BA")], "zoom", {"x":640,"y":360,"zoom":1,"rotation":0}, None, "A aparência não responde tudo"),
    shot("invisivel-micro", 1, [media("micro-fundo","svg","media/stage16/micro-world.svg",0,0,1280,720),
         native("micro-atom","atom",520,200,240,240,"#E9BF75",motion="pop"),
         t("text","micro-tag","MUNDO INVISÍVEL · VISUALIZAÇÃO ILUSTRATIVA",90,88,1100,40,20,"#EAF1EA")], "dissolve", {"x":640,"y":360,"zoom":1.1,"rotation":0}, None, "Aproximação ilustrativa"),
    shot("invisivel-comparacao", 2, [shape("invisivel-fundo-b",0,0,1280,720,"#0B1922"),
         prem("comparacao-aparencia", "comparison-pack-01", {"left-title":"O QUE OS OLHOS VEEM","right-title":"O QUE PRECISA SER AVALIADO","left-value":"CLAREZA","right-value":"QUALIDADE","left-description":"A aparência é apenas uma observação visual.","right-description":"Parâmetros e monitoramento exigem análise."},
              {"left":"#15333B","right":"#1A2D3B","primary":"#56CDBB","secondary":"#83AAB1","accent":"#4CC7B7","highlight":"#E4BE79","text":"#EDF4EF","muted":"#9CB6B9"},2),
         media("clip-agua","video","media/stage16/water-current-original.mp4",88,552,1104,100,video={"sourceInSeconds":0.5,"durationFrames":96,"speed":1.25,"crop":{"x":0.05,"y":0.08,"width":0.9,"height":0.84}})],
         "dissolve", {"x":640,"y":360,"zoom":1,"rotation":0}, None, "Comparação editorial e mídia original gerada localmente"),
], "O que os olhos não mostram")

add_scene("tratamento", [
    "Em muitos sistemas, o tratamento reúne barreiras diferentes para melhorar a qualidade da água.",
    "Partículas podem ser agrupadas e separadas; depois, a água pode passar por filtração e desinfecção.",
    "A sequência exata depende das características da água de origem e do sistema."], [
    shot("tratamento-diagrama",0,[shape("tratamento-fundo-a",0,0,1280,720,"#0B2028"), media("tratamento-infografico","svg","media/stage16/treatment.svg",0,0,1280,720),
         t("text","tratamento-head","BARREIRAS EM SEQUÊNCIA",78,66,800,54,32,"#F1F5EF"), native("flow-processo","flowchart",1000,95,145,110,"#E3BA70",opacity=0)],"slide",{"x":640,"y":360,"zoom":1,"rotation":0},[
         {"id":"revelar-etapas","at":{"cue":0,"cueProgress":0.38},"action":"show","target":"flow-processo"},
         {"id":"som-transicao-tratamento","at":{"cue":0,"cueProgress":0.44},"action":"sound","source":"media/stage16/transition-original.wav","gain":0.12}],"Etapas sem cartões repetidos"),
    shot("tratamento-transforma",1,[shape("tratamento-fundo-b",0,0,1280,720,"#0B2028"),native("flask-tratamento","flask",220,220,200,260,"#58CCBD"),
         native("network-tratamento","network",720,220,250,210,"#E3BA70"), t("text","tratamento-fluxo","AGRUPAR  →  SEPARAR  →  FILTRAR",155,510,980,48,30,"#F0F4EE"),
         t("text","tratamento-no-universal","ESQUEMA DIDÁTICO · NEM TODO SISTEMA USA A MESMA SEQUÊNCIA",130,620,1040,30,14,"#9AB7B7")],"dissolve",{"x":640,"y":360,"zoom":1.12,"rotation":0},[
         {"id":"aproxima-processo","at":{"cue":1,"cueProgress":0.28},"action":"camera","durationFrames":24,"x":620,"y":360,"zoom":1.18}],"Movimento de câmera durante a explicação"),
    shot("tratamento-numero",2,[shape("tratamento-fundo-c",0,0,1280,720,"#0B2028"),
         prem("numero-etapas","number-emphasis-01",{"value":"5","unit":"ETAPAS CITADAS","description":"Uma sequência comum descrita pelo CDC; sistemas variam conforme a fonte.","source":"CDC · How Water Treatment Works"},
              {"surface":"#142A31","accent":"#59CDBD","text":"#EDF4EF","muted":"#A5BCB9","secondary":"#E2BA70"},2),
         t("text","numero-nao-universal","EXEMPLO DE SEQUÊNCIA COMUM · NÃO UNIVERSAL",155,633,970,30,16,"#9FB8B8")],"cut",None,None,"Número contextualizado pela fonte"),
],"As barreiras do tratamento")

add_scene("medicao", [
    "Mas o processo não termina quando a água parece limpa.",
    "Monitoramento envolve planejamento, coleta de amostras, análise e acompanhamento dos resultados.",
    "No padrão brasileiro, são considerados mais de cem parâmetros de potabilidade."], [
    shot("medicao-lab",0,[shape("medicao-fundo-a",0,0,1280,720,"#101A20"),native("lab-main","laboratory-scene",125,125,520,390,"#58CBBB"),native("flask-main","flask",770,270,170,230,"#E4BC73"),
         t("text","lab-note","LABORATÓRIO ILUSTRADO · SEM RESULTADOS REAIS",80,615,940,28,15,"#A9BABB")],"fade",{"x":640,"y":360,"zoom":1,"rotation":0},None,"Laboratório sem falsos resultados"),
    shot("medicao-dashboard",1,[shape("medicao-fundo-b",0,0,1280,720,"#0B1922"),native("dash-main","dashboard",160,175,300,240,"#55C9B8"),native("table-main","data-table",510,175,260,240,"#DDB56E"),native("ring-main","progress-ring",850,175,220,220,"#56CBB8"),
         t("text","dashboard-sim","PAINEL CONCEITUAL · SEM MEDIÇÕES",288,510,720,38,20,"#F0F4EF")],"dissolve",{"x":640,"y":360,"zoom":1,"rotation":0},[
         {"id":"dashboard-move","at":{"cue":1,"cueProgress":0.34},"action":"transform","target":"ring-main","durationFrames":18,"rotation":12,"scale":1.08}],"Indicadores declarados como simulação"),
    shot("medicao-100",2,[shape("medicao-fundo-c",0,0,1280,720,"#0B1922"),
         prem("callout-parametros","statistic-callout-pack-01",{"value":"100+","unit":"PARÂMETROS","description":"O padrão brasileiro reúne mais de cem parâmetros de potabilidade.","source":"Ministério da Saúde · Vigiagua"},
              {"surface":"#14282F","accent":"#59CDBD","highlight":"#E4BC73","text":"#F0F4EE","secondary":"#92B8B8","muted":"#A5B8B8"},2),
         t("text","dados-fonte-nota","O número descreve o padrão, não uma amostra medida neste filme.",170,628,940,28,15,"#A5B8B8")],"cut",None,None,"Um número oficial, com referência"),
],"Medir para acompanhar")

add_scene("distribuicao", [
    "Depois do tratamento, a água segue por reservatórios e redes de distribuição.",
    "Há uma infraestrutura inteira entre a estação, a rua, as casas e a torneira.",
    "Manter esse caminho em boas condições também faz parte do desafio."], [
    shot("distribuicao-cidade",0,[shape("cidade-fundo-a",0,0,1280,720,"#0A1720"),media("cidade-ilustrada","svg","media/stage16/city-network.svg",0,0,1700,720),
         native("predio-marca","building",260,270,160,240,"#58C9BA"),native("casa-marca","house",1220,320,190,180,"#E1B96F"),
         t("text","cidade-label","UMA REDE ENTRE A ESTAÇÃO E A TORNEIRA",72,70,990,48,27,"#F0F4EF")],"slide",{"x":250,"y":360,"zoom":1,"rotation":0},None,"A cidade cresce no travelling"),
    shot("distribuicao-travelling",1,[media("cidade-travel-svg","svg","media/stage16/city-network-wide.svg",0,0,2600,720),
         native("rede-alvo-1","factory",120,290,165,180,"#58C9BA"),native("rede-alvo-2","house",1450,340,170,165,"#E1B96F"),
         t("text","rede-a","ESTAÇÃO",80,520,260,42,20,"#EFF4EF"), t("text","rede-b","CASA",1430,520,220,42,20,"#EFF4EF")],"dissolve",
         {"x":230,"y":360,"zoom":1,"rotation":0,"path":{"safeMargin":0.07,"captionSafeBottom":0.15,"targets":[
             {"target":"rede-alvo-1","at":{"cue":1,"cueProgress":0},"padding":3.4,"reveal":"spatial"},
             {"target":"rede-alvo-2","at":{"cue":1,"cueProgress":0.82},"padding":3.4,"easing":"ease-in-out","reveal":"spatial"}]}},None,"Travelling espacial estação-casa"),
    shot("distribuicao-phone",2,[shape("phone-fundo",0,0,1280,720,"#0A1720"),
         t("text","phone-explica","A INFORMAÇÃO TAMBÉM PRECISA CHEGAR ÀS PESSOAS",85,235,600,155,34,"#F1F5EF"),
         prem("phone-vigiagua","smartphone-premium-01",{"screen-title":"ÁGUA","notification":"SIMULAÇÃO · SEM AMOSTRA","screen":"media/stage16/monitor-simulacao.png"},
              {"surface":"#142831","frame":"#9BB8BA","highlight":"#E4BC73","accent":"#54CBBB","screen":"#102634","notification":"#12343C","muted":"#A4BCBB","text":"#F2F5EF"},2)],"cut",{"x":640,"y":360,"zoom":1,"rotation":0},None,"Representação não oficial"),
],"A cidade invisível")

add_scene("responsabilidades", [
    "No Brasil, controle e vigilância da qualidade da água são responsabilidades complementares.",
    "O controle cabe ao serviço responsável pelo abastecimento; a vigilância é feita pelo setor de saúde.",
    "Coleta e análise de amostras ajudam a acompanhar se os padrões de potabilidade são atendidos."], [
    shot("responsa-contraste",0,[shape("resp-fundo-a",0,0,1280,720,"#101A21"),native("doc-controle","document",250,195,175,225,"#58C9BA"),native("lab-vigilancia","laboratory-scene",800,178,350,265,"#E1B96F"),
         t("text","controle-label","CONTROLE",190,470,330,48,30,"#F1F5EF"), t("text","vigilancia-label","VIGILÂNCIA",830,470,330,48,30,"#F1F5EF")],"fade",{"x":640,"y":360,"zoom":1,"rotation":0},None,"Duas responsabilidades complementares"),
    shot("responsa-lowerthird",1,[shape("resp-fundo-b",0,0,1280,720,"#101A21"),native("mapa-brasil","globe",220,170,200,200,"#57C7B9"),
         prem("identificacao-vigiagua","lower-third-editorial-01",{"title":"Vigiagua","subtitle":"Vigilância da qualidade da água","source":""},
              {"surface":"#172A31","accent":"#58C9BA","text":"#F1F5EF","muted":"#A4B9B8"},1)],"dissolve",{"x":640,"y":360,"zoom":1,"rotation":0},None,"Identificação editorial institucional"),
    shot("responsa-fonte",2,[shape("resp-fundo-c",0,0,1280,720,"#101A21"),
         prem("source-institucional","source-label-01",{"title":"REFERÊNCIAS INSTITUCIONAIS","detail":"CDC · Ministério da Saúde / Vigiagua"},
              {"surface":"#182B32","highlight":"#E4BC71","text":"#EFF4EF","muted":"#A2B9BA"},2)],"cut",None,None,"Referências sem atribuir dados inventados"),
],"Quem acompanha a água")

add_scene("primeiro-gole", [
    "É fácil esquecer tudo isso diante de um copo.",
    "Água potável não é apenas água transparente: depende de ciência, infraestrutura e cuidado contínuo.",
    "Uma jornada inteira acontece antes do primeiro gole. E o invisível também nos protege."], [
    shot("fecho-citacao",0,[shape("fecho-fundo-a",0,0,1280,720,"#081820"),
         prem("citacao-original","dramatic-quote-01",{"quote":"A transparência é só o começo da história.","attribution":"Uma ideia original para este documentário","source":"Não é uma citação institucional"},
              {"accent":"#58CCBE","text":"#F1F5EF","muted":"#9CB7B8"},0),media("copo-ilustrativo","svg","media/stage16/watershed.svg",0,0,1280,720,opacity=0.23)],"fade",{"x":640,"y":360,"zoom":1,"rotation":0},None,"Citação autoral identificada"),
    shot("fecho-agua",1,[shape("fecho-fundo-b",0,0,1280,720,"#081820"),native("copo-cena","flask",520,185,220,310,"#65CFC0",opacity=0),media("agua-circulo","svg","media/stage16/micro-world.svg",800,210,300,300,opacity=0.55),
         t("text","ultimo-respiro","CIÊNCIA  ·  INFRAESTRUTURA  ·  CUIDADO",145,560,990,38,22,"#DDEBE5")],"dissolve",{"x":640,"y":360,"zoom":1.1,"rotation":0},[
         {"id":"copo-aparece","at":{"cue":1,"cueProgress":0.35},"action":"show","target":"copo-cena"}],"Retorno ao gesto cotidiano"),
    shot("fecho-titulo",2,[shape("fecho-fundo-c",0,0,1280,720,"#081820"),
         prem("titulo-final","title-reveal-01",{"main-title":"ANTES DO PRIMEIRO GOLE","subtitle":"O invisível também nos protege.","editorial-id":"FIM · UMA JORNADA ATÉ A TORNEIRA"},
              {"accent":"#59CDBD","secondary":"#82A8AB","highlight":"#E3BC71","text":"#F0F5EF","muted":"#9FB9BA"},2),
         t("text","creditos","PROCESSOS GERAIS: CDC · MONITORAMENTO NO BRASIL: MINISTÉRIO DA SAÚDE",150,636,980,26,13,"#99B3B4")],"fade",{"x":640,"y":360,"zoom":1,"rotation":0},[
         {"id":"som-fim","at":{"cue":2,"cueProgress":0.28},"action":"sound","source":"media/stage16/drop-original.wav","gain":0.12}],"A imagem final fecha o percurso"),
],"Antes do primeiro gole")

# Recipe elements default to an 8-frame exit fade. If the next shot dissolves,
# fading every outgoing layer first creates an unintended blank interval.
# Let the shot transition own that handoff and keep the outgoing composition
# intact at its final frame.
def keep_transition_outgoing_visible(elements):
    for element in elements:
        element["exitFrames"] = 0
        if element.get("kind") == "group":
            keep_transition_outgoing_visible(element.get("children", []))

for scene in scenes:
    shots = scene["shots"]
    for index in range(len(shots) - 1):
        following_transition = shots[index + 1].get("transitionIn", "cut")
        transition_type = (following_transition.get("type", "cut")
                           if isinstance(following_transition, dict) else following_transition)
        if transition_type != "cut":
            keep_transition_outgoing_visible(shots[index].get("elements", []))

recipe = {"format":"studio-dark-production","version":3,"catalogVersion":1,"title":"ANTES DO PRIMEIRO GOLE — A jornada invisível da água potável",
          "voice":"auto","captions":True,"musicGain":0.018,"effectsGain":0.025,
          "canvas":{"width":1280,"height":720},"wordAnimation":{"preset":"highlight","durationFrames":7},
          "artDirection":{"backgroundColor":"#081820","glowColor":"#173A43","primaryColor":"#58CBBB","secondaryColor":"#E3BC71","textColor":"#F0F5EF","cardColor":"#142A32","captionBackgroundColor":"#07141C","fontFamily":"Georgia","titleSize":58,"bodySize":32,"captionSize":25,"captionPosition":"bottom","safeMargin":54},
          "sources":[
              {"title":"CDC — How Water Treatment Works","url":"https://www.cdc.gov/drinking-water/about/how-water-treatment-works.html"},
              {"title":"Ministério da Saúde — Vigiagua","url":"https://www.gov.br/saude/pt-br/composicao/svsa/saude-ambiental/vigiagua/vigiagua/"},
              {"title":"Ministério da Saúde — Monitoramento da qualidade da água","url":"https://www.gov.br/saude/pt-br/composicao/svsa/saude-ambiental/vigiagua/monitoramento-da-qualidade-da-agua"},
              {"title":"OMS — Guidelines for Drinking-water Quality","url":"https://www.who.int/publications/i/item/9789240121225"}],
          "scenes":scenes}
(ROOT / "examples" / "antes-do-primeiro-gole.production.json").write_text(json.dumps(recipe, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")

# The appendix renders all registered native IDs in category-sorted sheets, six per page.
caps = json.loads((ROOT / "docs" / "capabilities-v1.json").read_text(encoding="utf-8"))
assets = caps["nativeAssets"]
pages = [assets[i:i+6] for i in range(0, len(assets), 6)]
appendix_scenes = []
for pi, page in enumerate(pages):
    elems = [shape(f"appendix-bg-{pi}",0,0,1280,720,"#101920"),t("text",f"appendix-title-{pi}",f"BIBLIOTECA NATIVA · {pi+1:02d}/{len(pages):02d}",50,24,1000,40,24,"#F0F5EF")]
    for j, a in enumerate(page):
        col, row = j % 3, j // 3
        x, y = 90 + col * 400, 140 + row * 270
        elems.append(native(f"native-{a['id']}",a['id'],x+95,y,150,132,"#59CDBD",secondaryColor="#E3BC71"))
        elems.append(t("text",f"label-{a['id']}",f"{a['name']}  ·  {a['id']}",x,y+155,350,35,18,"#E2EAE7"))
        elems.append(t("text",f"category-{a['id']}",a['category'].upper(),x,y+190,350,26,13,"#8CB0B1"))
    appendix_scenes.append({"id":f"native-page-{pi+1:02d}","narration":[f"Página {pi+1} da biblioteca nativa, com seis recursos registrados."],
        "shots":[shot(f"native-sheet-{pi+1:02d}",0,elems,"cut",{"x":640,"y":360,"zoom":1,"rotation":0},None,f"Página {pi+1}")]})
appendix = {"format":"studio-dark-production","version":3,"catalogVersion":1,"title":"Apêndice técnico — cobertura dos 72 nativos",
    "voice":"auto","captions":False,"musicGain":0,"effectsGain":0,"canvas":{"width":1280,"height":720},"scenes":appendix_scenes}
(ART / "nativos-72.production.json").write_text(json.dumps(appendix,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print(f"assets={MEDIA} scenes={len(scenes)} appendixPages={len(pages)} nativeAssets={len(assets)}")
