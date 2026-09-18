# API Feira das Profissões

Interface gráfica idêntica ao `demo-api-sdk` (QtGui), porém controlando o Arduino do sketch `feira_das_profissoes.ino` via porta serial — **sem** a API do detector X-Panel.

## O que faz

| UI (igual ao demo) | Comportamento neste projeto |
|---|---|
| Conectar | Abre a porta serial (ex.: `COM4`) |
| Modo mecânico + Conectar | Reabre/confirma serial Arduino |
| Iniciar captura (Tomografia, 20 imgs) | Envia comando `5` (`girar20Vezes`) |
| Iniciar captura (outras qtds) | Loop: `disparaFonte` + `girarGraus(360/N)` |
| Radiografia | Loop: apenas `disparaFonte` |
| Parar | Interrompe o loop no PC |

## Protocolo serial (mesmo do `.ino`)

| Comando | Função |
|---------|--------|
| `1` + graus | `girarGraus` |
| `2` | `disparaFonte` (relé) |
| `3` | `girandoVolta` |
| `4` | `reiniciarContador` |
| `5` | `girar20Vezes` (20×18° + relé) |

Baud: **9600**.

## Requisitos

- Qt 6 (módulos: `core`, `gui`, `widgets`, `serialport`)
- Visual Studio 2022 **ou** Qt Creator / qmake
- Arduino com o sketch `feira_das_profissoes.ino` gravado

## Como compilar e rodar (Visual Studio)

### 0) Pré-requisitos (obrigatório)

O erro `QtMsBuild\Qt.props não foi encontrado` significa que falta a integração Qt no Visual Studio.

1. Instale o **Qt** (ex.: 6.10.3 com componente **MSVC 2022 64-bit**)  
   https://www.qt.io/download-qt-installer
2. No Visual Studio: **Extensions → Manage Extensions** → busque e instale **Qt Visual Studio Tools**
3. Reinicie o Visual Studio
4. Menu **Extensions → Qt VS Tools → Qt Versions** → **Add** e aponte para a pasta do kit, por exemplo:
   `C:\Qt\6.10.3\msvc2022_64`
5. Feche e reabra `ApiFeiraDasProfissoes.sln`

### 1) Abrir a solução

Abra `ApiFeiraDasProfissoes.sln` no Visual Studio.

Se o nome da versão cadastrada no Qt VS Tools for diferente de `6.10.3_msvc2022_64`, ajuste `QtInstall` em `QtGui\QtGui.vcxproj` (ou em propriedades do projeto → Qt Project Settings) para o **mesmo nome** que aparece em Qt Versions.

### 2) Dependências do linker (já configuradas no projeto)

Se ainda precisar conferir manualmente:

1. Botão direito em **QtGui** → **Propriedades**
2. **Vinculador (Linker)** → **Entrada** (ou **Geral**, conforme a versão)
3. Em **Dependências adicionais**, use:

```text
$(CoreLibraryDependencies);%(AdditionalDependencies);$(Qt_LIBS_);kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;odbc32.lib;odbccp32.lib;
```

### 3) Compilar em Release

1. No topo do Visual Studio, escolha **Release** e **x64**
2. Botão direito em **QtGui** → **Compilar** (ou **Build**)
3. O executável sai em:

```text
api-feira-dass-profissoes\QtGui\bin\x64\Release\QtGui.exe
```

### 4) Copiar as DLLs do Qt (`windeployqt`)

1. Pesquise no Windows: **Qt 6.10.3 (MSVC 2022 64-bit)** e abra o prompt do Qt
2. Rode (ajuste o caminho se necessário):

```bat
cd /d B:\GitHub\api-feira-dass-profissoes\QtGui\bin\x64\Release
windeployqt QtGui.exe
```

Isso copia as DLLs do Qt para a pasta do `.exe`.

### 5) Não copie `XLibDllKosti.dll`

Esse passo vale só para o **demo-api-sdk** (detector X-Panel).  
Neste projeto da feira **não** há API do detector — só Qt + porta serial.

### 6) Executar

1. Grave o sketch `arduino\feira_das_profissoes.ino` no Arduino
2. Conecte o cabo USB e anote a porta (`COM3`, `COM4`, …)
3. Abra `QtGui.exe`
4. No campo de conexão, digite a porta (ex.: `COM4`) e clique em **Conectar**
5. Selecione o dispositivo → modo **Arduino** → **Conectar** (mecânico), se precisar
6. Preencha pasta/prefixo e clique em **Iniciar captura**

## Alternativa: Qt Creator / qmake

```bash
cd QtGui
qmake ApiFeira.pro
nmake
```

## Estrutura

```
api-feira-dass-profissoes/
├── ApiFeiraDasProfissoes.sln
├── README.md
└── QtGui/
    ├── ApiFeira.pro
    ├── QtGui.ui          # mesma UI do demo-api-sdk
    ├── QtGui.{h,cpp}
    ├── FeiraWorker.{h,cpp}  # serial + funções do .ino
    ├── Utils.{h,cpp}
    └── assets/
```

## Diferença em relação ao demo-api-sdk

- **Não** usa `XSystem` / `XCommand` / `XAcquisition` nem a DLL do detector
- Worker renomeado para `FeiraWorker`, focado no protocolo do Arduino da feira
- O campo de “IP” aceita porta `COMx` (padrão `COM4`)
