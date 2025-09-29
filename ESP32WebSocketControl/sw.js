// sw.js

const CACHE_NAME = 'esp32-control-panel-cache-v1';
const FILES_TO_CACHE = [
  '/',
  '/index.html',
  '/css/styles.css',
  '/js/main.js',
  '/js/appState.js',
  '/js/uiUpdater.js',
  '/js/websocketService.js',
  'https://cdn.jsdelivr.net/npm/@picocss/pico@1/css/pico.min.css'
];

// Evento de instalação: abre o cache e armazena os arquivos do app shell.
self.addEventListener('install', (event) => {
  console.log('[Service Worker] Install event');
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => {
        console.log('[Service Worker] Caching app shell files');
        return cache.addAll(FILES_TO_CACHE);
      })
  );
});

// Evento de ativação: limpa caches antigos.
self.addEventListener('activate', (event) => {
  console.log('[Service Worker] Activate event');
  event.waitUntil(
    caches.keys().then((keyList) => {
      return Promise.all(keyList.map((key) => {
        if (key !== CACHE_NAME) {
          console.log('[Service Worker] Removing old cache', key);
          return caches.delete(key);
        }
      }));
    })
  );
  return self.clients.claim();
});

// Evento fetch: intercepta requisições de rede.
// Usa a estratégia "cache-first": serve do cache se disponível, senão busca na rede.
self.addEventListener('fetch', (event) => {
  // Ignora requisições que não são GET (ex: POST, etc.)
  if (event.request.method !== 'GET') {
    return;
  }
  
  // Ignora requisições WebSocket, pois elas não podem ser cacheadas.
  if (event.request.url.includes('/ws')) {
    return;
  }

  event.respondWith(
    caches.match(event.request)
      .then((response) => {
        if (response) {
          // Se encontrou no cache, retorna a resposta do cache
          // console.log('[Service Worker] Serving from cache:', event.request.url);
          return response;
        }
        // Se não encontrou no cache, busca na rede
        // console.log('[Service Worker] Fetching from network:', event.request.url);
        return fetch(event.request);
      })
  );
});
