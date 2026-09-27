#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

// Page HTML stockée en mémoire flash (PROGMEM)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>ESP32 LED</title>
  <style>
    body {
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      margin: 0;
      background: #1a1a1a;
      color: white;
    }

    h1 { margin-bottom: 2rem; }

    .btn {
      padding: 1rem 2.5rem;
      font-size: 1.2rem;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      margin: 0.5rem;
    }

    #btn-on  { background: #00cc66; color: white; }
    #btn-off { background: #cc3333; color: white; }
  </style>
</head>
<body>

  <h1>Contrôle LED</h1>

  <button class="btn" id="btn-on"  onclick="send('/on')">Allumer</button>
  <button class="btn" id="btn-off" onclick="send('/off')">Éteindre</button>

  <script>
    // Envoie une requête GET à l'ESP32 sur la route indiquée
    function send(route) {
      fetch(route).catch(console.error);
    }
  </script>

</body>
</html>
)rawliteral";

#endif
