# Architecture

## 機能（ざっくり）

- Import/Export（PNG/JPEG）
- 選択モード: Quad(4点) / Ellipse(楕円: 描画・移動・リサイズ・回転)
- 左ペイン: ズーム（ホイール）/ パン（中ボタン）
- 右ペイン: 補正結果プレビュー（回転表示）
- Brightness（描画時の色行列で調整）

## モジュール責務

- `app`（`src/app/main.cpp`）
  - 入口: `main.cpp`（`wWinMain`、初期化、メッセージループ）
  - ウィンドウ: `window_proc.*`（`WndProc` とメッセージ分岐）

- `modules`（`src/modules/*`）
  - `state.*`: UI ID 定数、レイアウト/選択用の構造体定義、グローバル状態
  - `ui.*`: 画面レイアウト計算、コントロール生成、Export
  - `view.*`: 画像のフィット計算、座標変換、ヒットテスト
  - `render.*`: 描画（ダブルバッファ、オーバーレイ、Brightness反映）
  - `corrector.*`: `g_correctedImage` の生成（Quad/Ellipse）
  - `geometry.*`: 線形方程式、ホモグラフィ、4点の並べ替え
  - `input_handlers.*`: 入力（Quad/Ellipse、ズーム/パン、スライダー）
  - `image_io.*`: Import/Load とロード後の状態リセット
  - `edit_apply.*`: EditBoxの数値適用

## 依存方向（方針）

- `state`, `geometry`: 下位（できるだけ他に依存しない）
- `corrector`: `state`, `geometry` に依存してよい（UIには依存しない）
- `ui`: `state` に依存（保存/レイアウト/コントロール）
- `view`: `state`, `ui` に依存してよい
- `render`: `state`, `ui`, `view`, `geometry` に依存してよい（参照中心）
- `app`: 集約点（必要に応じて全部に依存してよい）

