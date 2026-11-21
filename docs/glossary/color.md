# Glossário — color management / ICC

Termos usados por `docs/features/F-008.md`. Uma frase por termo;
sinônimos não são criados para o que o ArtPro+ já nomeia.

| Termo | Definição | Spec |
|---|---|---|
| **ICC Profile** | Arquivo binário (`.icc`/`.icm`) que descreve a transformação entre um espaço de cor de dispositivo e o espaço de cor de perfil (PCS, CIELab ou CIEXYZ). | F-008 |
| **CMM (Color Management Module)** | Motor de transformação de cor; o ArtPro+ usa `lcms.dll` (Little CMS) como engine interna, com fallback a `mscms.dll` (Windows Color System) para perfis do sistema. | F-008 |
| **PCS (Profile Connection Space)** | Espaço de referência intermediário (Lab ou XYZ) usado para encadear perfis ICC; o perfil XYZ padrão do ArtPro+ é `Enfocus XYZ Profile.icc` (D50). | F-008 |
| **Color Type** | Enumeração que identifica o espaço de cor de um objeto: CMYK, RGB, Lab, Gray, Spot, DeviceN, MultiChannel. | F-008 |
| **Category Type** | Enumeração que seleciona o nível (stack) de perfil ativo: Default (0), Override, DeviceLink, Proof. | F-008 |
| **Rendering Intent** | Regra de mapeamento de gamut ao converter entre perfis: Perceptual, RelativeColorimetric, Saturation, AbsoluteColorimetric. | F-008 |
| **Black Point Compensation** | Flag que compensa o ponto preto do perfil de origem para o de destino durante a conversão. | F-008 |
| **GSharedObject<GCICCProfile>** | Ponteiro compartilhado (com contagem de referência) para um perfil ICC ativo; retornado por `GetActiveICCProfile`. | F-008 |
| **SetLevelICCProfile** | Operação que define/substitui/empilha um perfil ICC em um nível (stack) específico dentro de `GCColorManipulation`. | F-008 |
| **FOGRA53** | Espaço de troca CMYK (Color Exchange Space) usado como referência neutra de mídia para comunicação de cor na produção impressa. | F-008 |
| **ISO 12647-2** | Norma de impressão offset que define as condições de impressão (papel, tinta, TVI) usadas como base dos perfis ICC do ArtPro+. | F-008 |
| **OutputIntent** | Entrada no PDF que declara o perfil ICC de destino (DestOutputProfile) usado para a conversão de cor na saída. | F-008 |
| **ICCBased** | Espaço de cor no PDF calibrado por um perfil ICC embutido. | F-008 |
| **Spot (tinta)** | Tinta nomeada (Pantone, etc.) com espaço alternativo (alternate space) e curva de transferência no PDF. | F-008 |
| **DeviceN** | Espaço de cor no PDF para N canais de tinta (incluindo spot), com funções de mistura. | F-008 |