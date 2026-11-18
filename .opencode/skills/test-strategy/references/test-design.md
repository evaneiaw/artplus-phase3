# Design de testes

## Matriz de casos por regra

Para cada regra numerada da spec:

| Caso | Entrada | Esperado |
|---|---|---|
| nominal | valor típico | resultado documentado |
| limite inferior | 0 / mínimo | aceito ou erro conforme spec |
| limite superior | máximo | aceito ou erro |
| inválido | negativo / vazio / tipo errado | erro explícito |
| unidade alternativa | pt em vez de mm | conversão correta |
| repetição | duas execuções | mesmo resultado (se idempotente) |

## Template Qt Test

```cpp
#include <QtTest>
// F-025 — Trim Marks
class TrimMarkTest : public QObject {
    Q_OBJECT
private slots:
    void offsetZero();
    void offsetNegativeFails();
    void allSidesFalse();
};

void TrimMarkTest::offsetZero() {
    TrimMark::Params p{0.0, 5.0, 0.25, true};
    auto mark = TrimMark::fromParams(p);
    QCOMPARE(mark.count(), 4);
}
```

## Fixtures

- PDF de entrada em `tests/fixtures/`; registre origem e SHA-256 em `docs/testing/`.
- Fixture gerada por script (`tests/fixtures/make_test_pdfs.py`) é reproduzível.
- Não edite fixture para o teste passar; crie uma nova.

## Determinismo

- Fonte do sistema pode variar: prefira fonte embutida no fixture.
- Não compare timestamp, caminho absoluto ou UUID.
- `QImage` compara com tolerância de pixel.
- Ordem de mapa não é observável; compare conteúdo ordenado.

## Cobertura

Mapeie cada regra da spec para pelo menos um teste. Regra sem teste vai para o relatório como lacuna — não invente cobertura.

## Comandos

```powershell
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests --output-on-failure
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests -R Editing --output-on-failure
```
