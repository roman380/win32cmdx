# 作業記録 #
## 2010-10-10(Sun) ##
### clipx ###
  * Vista/Win7の標準コマンドclipと名前が衝突するので、clipをclipxに改名した.
## 2010-03-31(WED) ##
### Makefile ###
  * TARGETマクロの使い方を改善し、新規コマンド追加を容易にした.
### delx ###
  * コマンドライン解析処理を加え、ドキュメントを整備した.
  * version 1.0([r62](https://code.google.com/p/win32cmdx/source/detail?r=62))としてリリースした.
## 2010-01-19(TUE) ##
### zipdump ###
  * Info-ZIPの資料を追加し、"version made by"の詳細データ出力にInfo-ZIPの場合の情報を加えた.
  * 出力内容を色々改善した.
  * version 1.1([r33](https://code.google.com/p/win32cmdx/source/detail?r=33))としてリリースした
## 2010-01-17(SUN) ##
### zipdump ###
  * ローカルファイルエントリ、セントラルディレクトリの構造ダンプを完成させた
  * エクストラフィールドを各ID毎に分けてバイトダンプした
  * version 1.0 ([r17](https://code.google.com/p/win32cmdx/source/detail?r=17)) としてリリースした
### clip ###
  * 初期ソースをコミットした
  * zipdumpと同様に、ソリューションを作成した

## 2010-01-16(SAT) ##
Visual C++ 2008 Express Edition でソリューションを作成する
  * ファイル＞新規作成＞既存のコードからプロジェクトを作成
  * プロジェクト名＝zipdump
  * アプリケーションの種類＝コンソールアプケーション
  * myconfigを設けて、リリース・デバッグ共通の条件マクロをそこへ記入した
    * `"_CRT_SECURE_NO_WARNINGS;WINVER=0x0500;_WIN32_WINNT=0x0500"`
  * ビルド警告が出るので、Wp64オプションとインクリメンタルリンクを外した
  * リリース版のランタイムを /MD から /MT に変更し、単独実行可能とする