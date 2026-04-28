<?php
// ============================================================
// AFP Sistemas — admin.php
// Painel de gestão de clientes
// Acesse: seudominio.com/automacao/admin.php
// ============================================================

// ── CONFIGURAÇÕES ───────────────────────────────────────────
// Troque a senha antes de subir ao servidor!
$senha_admin = "afp@admin2024";

// Lista de clientes — mantenha igual ao api.php
$clientes = [

  "afp-demo-k7x9m2p3" => [
    "slug"   => "demo",
    "nome"   => "AFP Sistemas",
    "slogan" => "Sistema de demonstração",
    "pin"    => "123456",
    "ativo"  => true,
  ],

  // "COLE-AQUI-A-API-KEY" => [
  //     "slug"   => "nome-do-cliente",
  //     "nome"   => "Nome do Cliente",
  //     "slogan" => "Automação do Templo",
  //     "pin"    => "000000",
  //     "ativo"  => true,
  // ],

];

$pasta_raiz = __DIR__ . "/dados";

// ── AUTENTICAÇÃO ────────────────────────────────────────────
session_start();

if (isset($_POST['senha'])) {
  if ($_POST['senha'] === $senha_admin) $_SESSION['afp_admin'] = true;
  else $erro_login = true;
}

if (isset($_GET['sair'])) {
  session_destroy();
  header('Location: admin.php');
  exit;
}

if (!isset($_SESSION['afp_admin'])):
?>
  <!DOCTYPE html>
  <html lang="pt-br">

  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AFP Sistemas — Admin</title>
    <style>
      * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
      }

      body {
        font-family: 'Segoe UI', sans-serif;
        background: #0a0a14;
        display: flex;
        align-items: center;
        justify-content: center;
        height: 100vh;
      }

      .card {
        background: #141420;
        border-radius: 20px;
        padding: 44px 40px;
        width: 360px;
        border: 1px solid rgba(0, 217, 255, 0.2);
      }

      .logo {
        font-size: 28px;
        font-weight: 700;
        color: #00d9ff;
        margin-bottom: 4px;
        letter-spacing: -0.5px;
      }

      .sub {
        font-size: 13px;
        color: #5f5f8a;
        margin-bottom: 36px;
      }

      label {
        display: block;
        font-size: 11px;
        font-weight: 600;
        color: #5f5f8a;
        text-transform: uppercase;
        letter-spacing: 1px;
        margin-bottom: 8px;
      }

      input {
        width: 100%;
        padding: 14px 16px;
        background: #0f0f1a;
        border: 1px solid rgba(0, 217, 255, 0.2);
        border-radius: 12px;
        color: #e0e0ff;
        font-size: 15px;
        margin-bottom: 20px;
        outline: none;
        transition: border-color 0.2s;
      }

      input:focus {
        border-color: #00d9ff;
      }

      button {
        width: 100%;
        padding: 14px;
        background: linear-gradient(135deg, #00d9ff, #ff00ff);
        border: none;
        border-radius: 12px;
        color: #000;
        font-size: 15px;
        font-weight: 700;
        cursor: pointer;
        transition: opacity 0.2s;
      }

      button:hover {
        opacity: 0.9;
      }

      .erro {
        color: #ff3366;
        font-size: 13px;
        text-align: center;
        margin-top: 14px;
      }
    </style>
  </head>

  <body>
    <div class="card">
      <div class="logo">AFP Sistemas</div>
      <div class="sub">Painel de gestão — acesso restrito</div>
      <form method="POST">
        <label>Senha do painel</label>
        <input type="password" name="senha" placeholder="••••••••" autofocus>
        <button type="submit">Entrar</button>
      </form>
      <?php if (isset($erro_login)): ?><p class="erro">Senha incorreta.</p><?php endif; ?>
    </div>
  </body>

  </html>
<?php
  exit;
endif;

// ── COLETA DADOS DOS CLIENTES ────────────────────────────────
$dados = [];
$agora = time();

foreach ($clientes as $api_key => $c) {
  $pasta  = $pasta_raiz . "/" . $c['slug'];
  $hb     = [];
  $estados = [];
  $online  = false;
  $ultimo  = '—';

  if (is_dir($pasta)) {
    $hb_file = $pasta . "/heartbeat.json";
    if (file_exists($hb_file)) {
      $hb = json_decode(file_get_contents($hb_file), true) ?? [];
      if (!empty($hb['timestamp'])) {
        $diff   = $agora - (int)$hb['timestamp'];
        $online = $diff < 60;
        if ($diff < 60)       $ultimo = "há {$diff}s";
        elseif ($diff < 3600) $ultimo = "há " . round($diff / 60) . " min";
        else                  $ultimo = "há " . round($diff / 3600) . "h";
      }
    }
    $est_file = $pasta . "/estados.json";
    if (file_exists($est_file)) {
      $estados = json_decode(file_get_contents($est_file), true) ?? [];
    }
  }

  $dados[] = [
    'api_key'  => $api_key,
    'slug'     => $c['slug'],
    'nome'     => $c['nome'],
    'slogan'   => $c['slogan'],
    'pin'      => $c['pin'],
    'ativo'    => $c['ativo'],
    'online'   => $online,
    'ultimo'   => $ultimo,
    'ip'       => $hb['ip'] ?? '—',
    'on'       => count(array_filter($estados, fn($v) => $v == 1)),
    'total'    => count($estados),
    'estados'  => $estados,
  ];
}

$total    = count($dados);
$online_n = count(array_filter($dados, fn($c) => $c['online']));
?>
<!DOCTYPE html>
<html lang="pt-br">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AFP Sistemas — Admin</title>
  <style>
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }

    body {
      font-family: 'Segoe UI', sans-serif;
      background: #0a0a14;
      color: #e0e0ff;
      min-height: 100vh;
    }

    .topbar {
      background: #141420;
      border-bottom: 1px solid rgba(0, 217, 255, 0.15);
      padding: 16px 32px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .topbar .brand {
      font-size: 18px;
      font-weight: 700;
      color: #00d9ff;
      letter-spacing: -0.3px;
    }

    .topbar .brand span {
      color: #a0a0d0;
      font-weight: 400;
      font-size: 13px;
      margin-left: 12px;
    }

    .sair {
      color: #ff3366;
      font-size: 13px;
      text-decoration: none;
    }

    .sair:hover {
      text-decoration: underline;
    }

    .container {
      max-width: 1000px;
      margin: 0 auto;
      padding: 32px 20px;
    }

    .resumo {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 16px;
      margin-bottom: 36px;
    }

    .resumo-card {
      background: #141420;
      border-radius: 16px;
      padding: 24px;
      border: 1px solid rgba(0, 217, 255, 0.12);
    }

    .resumo-card .valor {
      font-size: 36px;
      font-weight: 700;
    }

    .resumo-card .label {
      font-size: 11px;
      color: #5f5f8a;
      margin-top: 4px;
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    .v-cyan {
      color: #00d9ff;
    }

    .v-green {
      color: #00ff88;
    }

    .v-red {
      color: #ff3366;
    }

    .secao-titulo {
      font-size: 11px;
      color: #5f5f8a;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      font-weight: 600;
      margin-bottom: 16px;
    }

    .cliente-card {
      background: #141420;
      border-radius: 18px;
      padding: 24px;
      border: 1px solid rgba(0, 217, 255, 0.1);
      margin-bottom: 16px;
      transition: border-color 0.2s;
    }

    .cliente-card:hover {
      border-color: rgba(0, 217, 255, 0.3);
    }

    .cliente-top {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      margin-bottom: 20px;
    }

    .cliente-nome {
      font-size: 18px;
      font-weight: 600;
      color: #f0f0ff;
    }

    .cliente-slogan {
      font-size: 12px;
      color: #5f5f8a;
      margin-top: 3px;
    }

    .badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 5px 14px;
      border-radius: 20px;
      font-size: 12px;
      font-weight: 600;
    }

    .badge-online {
      background: rgba(0, 255, 136, 0.12);
      color: #00ff88;
    }

    .badge-offline {
      background: rgba(255, 51, 102, 0.12);
      color: #ff3366;
    }

    .badge-inativo {
      background: rgba(160, 160, 208, 0.1);
      color: #a0a0d0;
    }

    .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: currentColor;
    }

    .info-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 16px;
      margin-bottom: 20px;
    }

    .info-item .il {
      font-size: 10px;
      color: #5f5f8a;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      margin-bottom: 4px;
    }

    .info-item .iv {
      font-size: 14px;
      color: #c0c0e0;
      font-weight: 500;
    }

    .estados-wrap {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
      margin-bottom: 20px;
    }

    .est-item {
      padding: 3px 10px;
      border-radius: 6px;
      font-size: 11px;
      font-weight: 600;
    }

    .est-on {
      background: rgba(0, 255, 136, 0.12);
      color: #00ff88;
    }

    .est-off {
      background: rgba(30, 30, 50, 0.8);
      color: #3a3a5a;
    }

    .key-row {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .key-box {
      flex: 1;
      background: #0f0f1a;
      border: 1px solid rgba(0, 217, 255, 0.15);
      border-radius: 10px;
      padding: 10px 14px;
      font-family: monospace;
      font-size: 12px;
      color: #5f5f8a;
      cursor: pointer;
      transition: 0.2s;
      word-break: break-all;
    }

    .key-box:hover {
      border-color: #00d9ff;
      color: #00d9ff;
    }

    .key-label {
      font-size: 10px;
      color: #5f5f8a;
      white-space: nowrap;
    }

    .link-acesso {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background: rgba(0, 217, 255, 0.08);
      border: 1px solid rgba(0, 217, 255, 0.2);
      padding: 8px 16px;
      border-radius: 10px;
      font-size: 12px;
      color: #00d9ff;
      text-decoration: none;
      transition: 0.2s;
      margin-top: 8px;
    }

    .link-acesso:hover {
      background: rgba(0, 217, 255, 0.15);
    }

    @media (max-width: 600px) {
      .resumo {
        grid-template-columns: 1fr 1fr;
      }

      .info-grid {
        grid-template-columns: 1fr 1fr;
      }

      .topbar {
        padding: 14px 16px;
      }
    }
  </style>
</head>

<body>

  <div class="topbar">
    <div class="brand">
      AFP Sistemas
      <span>Painel Admin</span>
    </div>
    <a href="?sair" class="sair">Sair</a>
  </div>

  <div class="container">

    <div class="resumo">
      <div class="resumo-card">
        <div class="valor v-cyan"><?= $total ?></div>
        <div class="label">Clientes cadastrados</div>
      </div>
      <div class="resumo-card">
        <div class="valor v-green"><?= $online_n ?></div>
        <div class="label">Online agora</div>
      </div>
      <div class="resumo-card">
        <div class="valor v-red"><?= $total - $online_n ?></div>
        <div class="label">Offline</div>
      </div>
    </div>

    <div class="secao-titulo">Clientes</div>

    <?php foreach ($dados as $c): ?>
      <div class="cliente-card">

        <div class="cliente-top">
          <div>
            <div class="cliente-nome"><?= htmlspecialchars($c['nome']) ?></div>
            <div class="cliente-slogan"><?= htmlspecialchars($c['slogan']) ?> &nbsp;·&nbsp; <code style="font-size:11px;color:#5f5f8a"><?= htmlspecialchars($c['slug']) ?></code></div>
          </div>
          <?php if (!$c['ativo']): ?>
            <span class="badge badge-inativo"><span class="dot"></span> Inativo</span>
          <?php elseif ($c['online']): ?>
            <span class="badge badge-online"><span class="dot"></span> Online</span>
          <?php else: ?>
            <span class="badge badge-offline"><span class="dot"></span> Offline</span>
          <?php endif; ?>
        </div>

        <div class="info-grid">
          <div class="info-item">
            <div class="il">Último ping</div>
            <div class="iv"><?= htmlspecialchars($c['ultimo']) ?></div>
          </div>
          <div class="info-item">
            <div class="il">IP local</div>
            <div class="iv"><?= htmlspecialchars($c['ip']) ?></div>
          </div>
          <div class="info-item">
            <div class="il">Dispositivos ligados</div>
            <div class="iv"><?= $c['on'] ?> / <?= $c['total'] ?></div>
          </div>
          <div class="info-item">
            <div class="il">PIN de acesso</div>
            <div class="iv" style="font-family:monospace;letter-spacing:2px"><?= htmlspecialchars($c['pin']) ?></div>
          </div>
        </div>

        <?php if (!empty($c['estados'])): ?>
          <div class="estados-wrap">
            <?php foreach ($c['estados'] as $dev => $est): ?>
              <span class="est-item <?= $est ? 'est-on' : 'est-off' ?>"><?= htmlspecialchars($dev) ?></span>
            <?php endforeach; ?>
          </div>
        <?php endif; ?>

        <div class="key-row">
          <span class="key-label">API Key (ESP32):</span>
          <div class="key-box"
            onclick="navigator.clipboard.writeText('<?= htmlspecialchars($c['api_key']) ?>').then(()=>{this.textContent='✓ Copiado!';setTimeout(()=>this.textContent='<?= htmlspecialchars($c['api_key']) ?>',1500)})"
            title="Clique para copiar">
            <?= htmlspecialchars($c['api_key']) ?>
          </div>
        </div>

        <a class="link-acesso"
          href="index.html?slug=<?= urlencode($c['slug']) ?>"
          target="_blank">
          <i>&#8599;</i> Acessar painel do cliente
        </a>

      </div>
    <?php endforeach; ?>

  </div>

  <script>
    setTimeout(() => location.reload(), 30000);
  </script>
</body>

</html>