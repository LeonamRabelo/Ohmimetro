# 🔌 Ohmímetro com Reconhecimento Automático do Código de Cores — BitDogLab

# 📋 Descrição
Este projeto consiste na criação de um ohmímetro inteligente utilizando a placa BitDogLab (RP2040).
O sistema realiza a leitura de resistores através de um divisor de tensão, calcula seu valor, identifica o valor comercial mais próximo da série E24 (5%), e determina automaticamente as três primeiras faixas do código de cores.

Essas informações são exibidas no display OLED SSD1306 (128x64), incluindo:

- Valor numérico da resistência;

- Código de cores correspondente (1ª, 2ª faixa e multiplicador);

- Representação gráfica estilizada de um resistor com as faixas desenhadas.

Além de utilizar a matriz de LEDS para representar visualmente as cores das faixas do resistor.

# 🎯 Funcionalidades
**📈 Leitura da resistência usando o ADC do RP2040.** 
**🎯 Aproximação automática para o valor E24 mais próximo.**
**🎨 Conversão para o código de cores padrão dos resistores.**
**🖥️ Exibição no display OLED (SSD1306):**
- Valor do resistor;
- Nome das cores de cada faixa;
- Desenho de um resistor com marcação das faixas.

**Matriz de LEDS representando as cores das faixas do resistor em suas colunas centrais**
**🔄 Média de 500 leituras ADC para maior estabilidade.**

# 🖥️ Visualização no Display
**Topo: Valor da resistência em ohms**
**Centro: Desenho de um resistor com faixas estilizadas:**
- Faixa 1 (1º dígito)
- Faixa 2 (2º dígito)
- Multiplicador (potência)

**Base: Legenda com as cores correspondentes.**

# ⚠️ Observação Importante
Atenção: No simulador Wokwi, a leitura do pino ADC 28 pode não funcionar corretamente, retornando valor 0, logo para simular, podemos passar o valor do resistor de forma direta.
Para testes físicos reais, descomentar a linha de leitura do ADC e comentar a linha de valor fixo no código.
Outro ponto é a cor preta na matriz de LEDS que realmente fica com a coluna apagada, já que seu valor rgb = {0,0,0}

![Simulação no Wokwi](image-2.png)

**Link para vídeo explicativo:** https://youtu.be/2itwI0OStjM

# Autor
Leonam S. Rabelo