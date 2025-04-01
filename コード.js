// 定時になると新しいgraphシートを作成し、ESP32からデータを受信して記録しつつグラフを更新するスクリプト

// webアプリのgetリクエストが実行された際に走る関数
// M5toughからデータを受け取る
function doGet(e) {
  Logger.log("doGet function triggered"); // Logger.logを使用すると実行ログに記録を残せる
  let id = '1mabuD7KxA_UR3nkZQ0L_DZaaisZYHqszRFvGvkFV8W0'; // スプレッドシートIDを指定

  // 現在の日時を取得
  let now = new Date();
  // 年月日時のフォーマットでシート名を作成
  let sheetName = Utilities.formatDate(now, Session.getScriptTimeZone(), "yyyyMMdd");

  let result;

  try {
    // e.parameterの検証
    if (!e.parameter || !e.parameter.time || !e.parameter.temperature) {
      result = 'Error: Missing parameters';
      return ContentService.createTextOutput(result);
    }

    // シートを取得または作成
    let spreadsheet = SpreadsheetApp.openById(id);
    Logger.log(spreadsheet);
    let sheet = spreadsheet.getSheetByName(`${sheetName}_graph`);
    if (!sheet) {
      sheet = spreadsheet.insertSheet(`${sheetName}_graph`);
      sheet.getRange("A1").setValue("Time");
      sheet.getRange("B1").setValue("Temperature");
    }

    // 重複データを防ぐ
    let existingTimeStamps = sheet.getRange(2, 1, sheet.getLastRow() - 1, 1).getValues().flat();
    if (existingTimeStamps.includes(e.parameter.time)) {
      result = "Duplicate data ignored";
      return ContentService.createTextOutput(result);
    }

    // 新しい行を挿入
    let newRow = sheet.getLastRow() + 1;
    let rowData = [
      e.parameter.time || "N/A",                              // タイムスタンプ
      parseFloat(e.parameter.temperature) || 0.0,            // 温度
    ];
    sheet.getRange(newRow, 1, 1, rowData.length).setValues([rowData]);

    // グラフの範囲を更新
    let charts = sheet.getCharts();
    for (let chart of charts) {
      if (chart.getType() === Charts.ChartType.LINE) {
        let range = sheet.getRange(1, 1, sheet.getLastRow(), 3); // 最終行までの範囲を取得
        chart = chart.modify()
          .clearRanges()
          .addRange(range)
          .build();
        sheet.updateChart(chart);
      }
    }

    result = 'Ok';
  } catch (error) {
    result = `Error: ${error.message}`;
  }

  // 時間から通知するかを判定 17時以降であれば通知する
  // const nowHour = parseInt(now.getHours());
  // if (nowHour >= 17) {
  //   // 温度が特定の範囲から外れたら通知
  //   if (parseFloat(e.parameter.temperature) <= 70) {
  //     const message = `温度を確認してください\n温度: ${e.parameter.temperature}`;
  //     sendLineMessage("U933ccb6a8b8f33062b24743451370037", message);
  //   }
  // }

  return ContentService.createTextOutput(result);
}

// line webhookに反応するようにメッセージがあったらPOST requestに反応する
function doPost(e) {
  // リクエストボディをパース
  const json = JSON.parse(e.postData.contents);

  // 最新の温度を取得
  const temp = getLastCellValueInSpecificSheet();

  // LINEのイベントデータを取得
  const event = json.events[0];

  // UUID（ユーザーID）とグループIDの取得
  const userId = event.source.userId || "ユーザーIDが取得できませんでした";
  const groupId = event.source.groupId || "グループIDがありません（個人チャット）";

  // メッセージ内容を取得
  const messageText = json.events[0].message.text;

  // グループに招待した際に全員のメッセージに反応していたらうざいので
  // 特定のユーザーグループからのメッセージに対してのみ反応するようにしている
  const adminUIDs = [
    "U933ccb6a8b8f33062b24743451370037",
    "U6dedf3997fd60273eb89b1d57cf3a6eb",
    "U6a1a533f729727d1f5236212d85cfd26",
    "U81e0d335285af19c20f83dd2b20edca3",
  ];

  const groupUID = 'Cd9c54dbeab78afef334da0c07dd400c0';

  // if (adminUIDs.includes(userId)) {
  //   let message = `ユーザーID: ${userId}\nグループID: ${groupId}`;

  //   // 送信されたテキストの内容によってメッセージを出し分ける
  //   if (messageText == '温度') {
  //     message = `温度: ${temp}`;
  //   }

  //   sendLineMessage(userId, message);
  // }

  if (adminUIDs.includes(userId) && (groupId != groupUID)) {
    let message = `ユーザーID: ${userId}\nグループID: ${groupId}`;

    // 送信されたテキストの内容によってメッセージを出し分ける
    if (messageText == '温度') {
      message = `温度: ${temp}`;
    }

    sendLineMessage(userId, message);
  }

  if (groupId == groupUID) {
    // 送信されたテキストの内容によってメッセージを出し分ける
    if (messageText == '温度') {
      message = `温度: ${temp}`;
    }
    sendLineMessage(groupId, message);
  }

  return ContentService.createTextOutput("ok");
}

// メッセージを送信
function sendLineMessage(userId, messageText) {
  const accessToken = 'ScIucVk/rvnetDPO5Ww+Tfd19vkuLmNJx2fslSX2+SgT7y6BeMZwijxqbdG/C9tpa0b0Qj1XNjrSMH+XQqAoPWeSALgvKyZbq+bblWZDZoUaHcbqtt79LHskmKYHCRrOR2k2Gxo0zyI1FY5anGrUSgdB04t89/1O/w1cDnyilFU='; // チャネルアクセストークン

  const url = 'https://api.line.me/v2/bot/message/push';
  const payload = {
    to: userId,
    messages: [
      {
        type: 'text',
        text: messageText,
      },
    ],
  };

  const options = {
    method: 'post',
    headers: {
      'Content-Type': 'application/json',
      Authorization: `Bearer ${accessToken}`,
    },
    payload: JSON.stringify(payload),
  };

  const response = UrlFetchApp.fetch(url, options);
  Logger.log(response.getContentText()); // レスポンスをログに記録
}

// 最新の温度を取得して返す
function getLastCellValueInSpecificSheet() {
  // スプレッドシートを取得
  const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();

  // 現在の日時データを取得
  let now = new Date();
  let sheetDate = Utilities.formatDate(now, Session.getScriptTimeZone(), "yyyyMMdd");


  // 特定のシート（タブ）を取得
  const sheetName = `${sheetDate}_graph`; // シート名を指定
  const sheet = spreadsheet.getSheetByName(sheetName);

  if (!sheet) {
    Logger.log(`シート「${sheetName}」が見つかりません`);
    return null;
  }

  // 温度の列を指定 (列B)
  const column = 2; // 列Bは2
  const lastRow = sheet.getLastRow(); // シート内の最終行を取得

  // 特定の列の値をすべて取得
  const values = sheet.getRange(1, column, lastRow).getValues();

  // 下から上に向かって値が存在する最初のセルを探す
  for (let i = values.length - 1; i >= 0; i--) {
    if (values[i][0] !== "") { // 値が空でないセルを探す
      Logger.log(`最も下の行の値: ${values[i][0]}`);
      return values[i][0]; // 最初に見つかった値を返す
    }
  }

  // 値が見つからない場合
  Logger.log("指定した列には値がありません");
  return null;
}

// トリガーからの実行でグラフ作成までの改善コード
function createNewSheetWithGraph() {
  const spreadsheet = SpreadsheetApp.openById("1mabuD7KxA_UR3nkZQ0L_DZaaisZYHqszRFvGvkFV8W0");
  let now = new Date();
  let sheetName = Utilities.formatDate(now, "Asia/Tokyo", "yyyyMMdd");

  // シート名が既に存在するか確認
  if (!spreadsheet.getSheetByName(`${sheetName}_graph`)) {
    let newSheet = spreadsheet.insertSheet(`${sheetName}_graph`);

    // タイムゾーンを明示的に指定
    SpreadsheetApp.flush();

    // データの初期設定
    newSheet.getRange("A1").setValue("Time");
    newSheet.getRange("B1").setValue("Temperature");
    newSheet.getRange("A2").setValue("00:00");
    newSheet.getRange("B2").setValue("0");

    // チャート範囲を明確に定義
    let range = newSheet.getRange("A1:B2");

    // チャート作成を遅延させる
    Utilities.sleep(1000); // 1秒待機

    try {
      let chart = newSheet.newChart()
        .setChartType(Charts.ChartType.LINE)
        .addRange(range)
        .setPosition(1, 4, 0, 0)
        .setOption('title', 'Temperature Over Time')
        .setOption('legend', { position: 'bottom' })
        .setOption('width', 800)
        .setOption('height', 400)
        .build();

      // チャートの挿入を明示的に行う
      newSheet.insertChart(chart);

      // 追加のログ出力
      Logger.log("Chart created successfully");
    } catch (e) {
      Logger.log("Error creating chart: " + e.toString());
    }

    // 最終的な変更を確定
    SpreadsheetApp.flush();
  }

  setMiniTrigger();
}

// 時間単位で設定していたトリガーの実行タイミングを設定
function setMiniTrigger() {
  let triggers = ScriptApp.getProjectTriggers();
  for(let trigger of triggers) {
    let funcName = trigger.getHandlerFunction();

    // もしcreateNewSheetWithGraphの名前でトリガーが設定されていたら編集後初回実行分を削除して新たにトリガーをコードから設定
    if (funcName == 'createNewSheetWithGraph') {
      ScriptApp.deleteTrigger(trigger);
    }
  }

  let now = new Date();
  let y = now.getFullYear();
  let m = now.getMonth();
  let d = now.getDate();
  let date = new Date(y, m, d + 1, 09, 00); // dに＋1しているのは翌日の実行トリガーなので今日+1という実装
  ScriptApp.newTrigger('createNewSheetWithGraph').timeBased().at(date).create();
}

// 毎日午前0:00〜1:00にトリガーを設定する関数
function setDailyTrigger() {
  ScriptApp.newTrigger('createNewSheetWithGraph')
    .timeBased()
    .everyDays(1)
    .atHour(0) // 午前0時にトリガー実行
    .create();
}