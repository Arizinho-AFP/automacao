<?php
// ============================================================
// AFP Sistemas — api.php
// Sistema de Automação Comercial — Versão Genérica
// ============================================================
header('Access-Control-Allow-Origin: *');
header('Content-Type: application/json; charset=utf-8');

// ============================================================
// CADASTRO DE CLIENTES
// ─────────────────────────────────────────────────────────────
// Adicione um bloco por cliente instalado.
//
// slug     → identificador único (sem espaços/acentos)
// nome     → nome exibido no painel
// slogan   → subtítulo do painel (ex: "Automação Residencial")
// pin      → PIN de acesso ao painel remoto
// api_key  → chave gravada no firmware do ESP32 do cliente
//            gere em: https://www.uuidgenerator.net/
// ativo    → false desativa sem apagar os dados
// ============================================================
$clientes = [

    "afp-demo-k7x9m2p3" => [
        "slug"   => "demo",
        "nome"   => "AFP Sistemas",
        "slogan" => "Sistema de demonstração",
        "pin"    => "123456",
        "ativo"  => true,
    ],

    // ── Modelo para novo cliente ───────────────────────────
    // "COLE-AQUI-A-API-KEY-GERADA" => [
    //     "slug"   => "nome-do-cliente",
    //     "nome"   => "Nome do Cliente",
    //     "slogan" => "Automação do Templo",
    //     "pin"    => "000000",
    //     "ativo"  => true,
    // ],

];

// ============================================================
// MAPA DE GRUPOS
// Define quais dispositivos pertencem a cada grupo.
// O firmware e o painel usam esses grupos para comandos em lote.
// ============================================================
$mapa_grupos = [
    'luzes'        => ['luz_01', 'luz_02', 'luz_03', 'luz_04', 'luz_05', 'luz_06', 'luz_07', 'luz_08'],
    'ventiladores' => ['vent_01', 'vent_02', 'vent_03', 'vent_04', 'vent_05', 'vent_06', 'vent_07', 'vent_08', 'vent_09'],
    'ares'         => ['ar_01', 'ar_02', 'ar_03', 'ar_04', 'ar_05'],
    'externos'     => ['refletor_dir', 'refletor_esq', 'logo', 'fachada'],
    'tomadas'      => ['tomada_01', 'tomada_02', 'tomada_03', 'tomada_04'],
];

// ============================================================
// CONFIGURAÇÕES INTERNAS
// ============================================================
$pasta_raiz    = __DIR__ . "/dados";
$token_expiry  = 86400; // 24 horas

// ============================================================
// FUNÇÕES AUXILIARES
// ============================================================

function ler_json(string $arquivo, $padrao = [])
{
    if (!file_exists($arquivo)) return $padrao;
    $dados = json_decode(file_get_contents($arquivo), true);
    return is_array($dados) ? $dados : $padrao;
}

function salvar_json(string $arquivo, $dados): bool
{
    $fp = fopen($arquivo, 'c+');
    if (!$fp) return false;
    if (flock($fp, LOCK_EX)) {
        ftruncate($fp, 0);
        rewind($fp);
        fwrite($fp, json_encode($dados, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
        flock($fp, LOCK_UN);
    }
    fclose($fp);
    return true;
}

function resposta(array $dados, int $codigo = 200): void
{
    http_response_code($codigo);
    echo json_encode($dados, JSON_UNESCAPED_UNICODE);
    exit;
}

function erro(string $msg, int $codigo = 400): void
{
    resposta(['erro' => $msg], $codigo);
}

function gerar_token(): string
{
    return bin2hex(random_bytes(32));
}

function inicializar_pasta(string $pasta): void
{
    if (!is_dir($pasta)) mkdir($pasta, 0755, true);
    $estados = $pasta . "/estados.json";
    $agenda  = $pasta . "/agenda.json";
    if (!file_exists($estados)) salvar_json($estados, []);
    if (!file_exists($agenda))  salvar_json($agenda,  []);
}

function validar_token(string $pasta, int $expiry): bool
{
    $token = $_SERVER['HTTP_X_SESSION_TOKEN'] ?? $_GET['session_token'] ?? '';
    if (empty($token)) return false;
    $tokens = ler_json($pasta . "/tokens.json", []);
    if (!isset($tokens[$token])) return false;
    return (time() - (int)$tokens[$token]['criado_em']) < $expiry;
}

function registrar_heartbeat(string $pasta): void
{
    salvar_json($pasta . "/heartbeat.json", [
        'ultimo_ping' => date('Y-m-d H:i:s'),
        'ip'          => $_SERVER['REMOTE_ADDR'] ?? '',
        'timestamp'   => time(),
    ]);
}

function aplicar_comando(array &$estados, string $id, int $val, array $grupos): void
{
    if (!preg_match('/^[a-z0-9_]+$/', $id)) erro("ID inválido.");
    if (array_key_exists($id, $grupos)) {
        foreach ($grupos[$id] as $dev) $estados[$dev] = $val;
    } else {
        $estados[$id] = $val;
    }
}

// ============================================================
// ROTEAMENTO
// ============================================================
$rota   = $_GET['rota'] ?? 'estados';
$metodo = $_SERVER['REQUEST_METHOD'];

// ────────────────────────────────────────────────────────────
// ROTA: LOGIN
// POST api.php?rota=login
// Body: slug=demo&pin=123456
// Retorna: { token, slug, nome, slogan }
// ────────────────────────────────────────────────────────────
if ($rota === 'login' && $metodo === 'POST') {
    $slug_env = trim($_POST['slug'] ?? '');
    $pin_env  = trim($_POST['pin']  ?? '');

    $cliente_ok  = null;
    foreach ($clientes as $key => $c) {
        if ($c['slug'] === $slug_env && $c['ativo']) {
            $cliente_ok = $c;
            break;
        }
    }

    if (!$cliente_ok) erro("Cliente não encontrado.", 404);

    if ($pin_env !== $cliente_ok['pin']) {
        sleep(1); // anti brute-force
        erro("PIN incorreto.", 401);
    }

    $pasta = $pasta_raiz . "/" . $cliente_ok['slug'];
    inicializar_pasta($pasta);

    $token  = gerar_token();
    $tokens = ler_json($pasta . "/tokens.json", []);

    // Limpa tokens expirados
    foreach ($tokens as $t => $info) {
        if ((time() - (int)$info['criado_em']) > $token_expiry) unset($tokens[$t]);
    }

    $tokens[$token] = ['criado_em' => time(), 'ip' => $_SERVER['REMOTE_ADDR'] ?? ''];
    salvar_json($pasta . "/tokens.json", $tokens);

    resposta([
        'status' => 'sucesso',
        'token'  => $token,
        'slug'   => $cliente_ok['slug'],
        'nome'   => $cliente_ok['nome'],
        'slogan' => $cliente_ok['slogan'],
    ]);
}

// ────────────────────────────────────────────────────────────
// ROTAS DO ESP32 — autenticadas por api_key
// Header ou GET: key=API_KEY_DO_CLIENTE
// ────────────────────────────────────────────────────────────
$api_key   = $_GET['key'] ?? $_SERVER['HTTP_X_API_KEY'] ?? '';
$cliente_esp = null;

if (!empty($api_key)) {
    foreach ($clientes as $key => $c) {
        if ($key === $api_key && $c['ativo']) {
            $cliente_esp = $c;
            break;
        }
    }
}

if ($cliente_esp) {
    $pasta   = $pasta_raiz . "/" . $cliente_esp['slug'];
    inicializar_pasta($pasta);
    $estados_file = $pasta . "/estados.json";
    $agenda_file  = $pasta . "/agenda.json";

    // GET estados → ESP32 sincroniza
    if ($rota === 'estados' && $metodo === 'GET') {
        registrar_heartbeat($pasta);
        echo file_get_contents($estados_file);
        exit;
    }

    // POST estados → ESP32 envia comando
    if ($rota === 'estados' && $metodo === 'POST') {
        $id    = trim($_POST['id']     ?? '');
        $val   = (int)($_POST['estado'] ?? -1);
        if ($id === '' || $val < 0) erro("Parâmetros inválidos.");
        $estados = ler_json($estados_file, []);
        aplicar_comando($estados, $id, $val, $mapa_grupos);
        salvar_json($estados_file, $estados);
        resposta(['status' => 'ok']);
    }

    // GET agenda → ESP32 puxa agenda
    if ($rota === 'agenda' && $metodo === 'GET') {
        echo file_exists($agenda_file) ? file_get_contents($agenda_file) : '[]';
        exit;
    }

    // POST agenda → ESP32 salva agenda
    if ($rota === 'agenda' && $metodo === 'POST') {
        $corpo = file_get_contents('php://input');
        if (json_decode($corpo) === null) erro("JSON inválido.");
        file_put_contents($agenda_file, $corpo);
        resposta(['status' => 'agenda_salva']);
    }

    erro("Rota não encontrada.", 404);
}

// ────────────────────────────────────────────────────────────
// ROTAS DO PAINEL WEB — autenticadas por session_token
// Header: X-Session-Token: <token>
// GET/POST param: slug=<slug>
// ────────────────────────────────────────────────────────────
$slug_web = trim($_GET['slug'] ?? $_POST['slug'] ?? '');

if (!empty($slug_web)) {
    if (!preg_match('/^[a-z0-9\-]+$/', $slug_web)) erro("Slug inválido.");

    $pasta = $pasta_raiz . "/" . $slug_web;

    if (!is_dir($pasta)) erro("Cliente não encontrado.", 404);
    if (!validar_token($pasta, $token_expiry)) erro("Sessão inválida. Faça login novamente.", 401);

    $estados_file = $pasta . "/estados.json";
    $agenda_file  = $pasta . "/agenda.json";

    // GET estados
    if ($rota === 'estados' && $metodo === 'GET') {
        echo file_exists($estados_file) ? file_get_contents($estados_file) : '{}';
        exit;
    }

    // POST estados
    if ($rota === 'estados' && $metodo === 'POST') {
        $id  = trim($_POST['id']      ?? '');
        $val = (int)($_POST['estado'] ?? -1);
        if ($id === '' || $val < 0) erro("Parâmetros inválidos.");
        $estados = ler_json($estados_file, []);
        aplicar_comando($estados, $id, $val, $mapa_grupos);
        salvar_json($estados_file, $estados);
        resposta(['status' => 'ok']);
    }

    // GET agenda
    if ($rota === 'agenda' && $metodo === 'GET') {
        echo file_exists($agenda_file) ? file_get_contents($agenda_file) : '[]';
        exit;
    }

    // POST agenda
    if ($rota === 'agenda' && $metodo === 'POST') {
        $corpo = file_get_contents('php://input');
        if (json_decode($corpo) === null) erro("JSON inválido.");
        file_put_contents($agenda_file, $corpo);
        resposta(['status' => 'agenda_salva']);
    }

    erro("Rota não encontrada.", 404);
}

// ────────────────────────────────────────────────────────────
// ROTA: INFO PÚBLICA DO CLIENTE (para a tela de login)
// GET api.php?rota=info&slug=demo
// Retorna: { nome, slogan } — sem dados sensíveis
// ────────────────────────────────────────────────────────────
if ($rota === 'info' && $metodo === 'GET') {
    $slug_info = trim($_GET['slug'] ?? '');
    foreach ($clientes as $c) {
        if ($c['slug'] === $slug_info && $c['ativo']) {
            resposta(['nome' => $c['nome'], 'slogan' => $c['slogan']]);
        }
    }
    erro("Cliente não encontrado.", 404);
}

erro("Requisição inválida.", 400);
