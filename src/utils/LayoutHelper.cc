#include "LayoutHelper.h"

// MVP-layout helper.
// Идея простая: все SSR-страницы (лендинг, публичная форма, кабинеты) используют единый хедер/футер и один CSS.
// Потом это можно заменить на шаблонизатор/компоненты, но сейчас так меньше дублирования и «визуального бардака».

namespace layout {

static std::string authBlockHtml(const std::optional<std::string> &role) {
    // По умолчанию гость видит кнопку «Личный кабинет», а авторизованный — «Выход».
    if (!role.has_value()) {
        return "<a class='btn btn-outline' href='/cabinet'>Личный кабинет</a>";
    }

    if (*role == "manager") {
        return "<form method='POST' action='/manager/logout' class='inline'>"
               "<button type='submit' class='btn btn-outline'>Выход</button>"
               "</form>";
    }
    if (*role == "partner") {
        return "<form method='POST' action='/partner/logout' class='inline'>"
               "<button type='submit' class='btn btn-outline'>Выход</button>"
               "</form>";
    }

    // неизвестная роль — ведём себя как гость
    return "<a class='btn btn-outline' href='/cabinet'>Личный кабинет</a>";
}

std::string renderHeader(const std::optional<std::string> &role) {
    // Важно: здесь начинается HTML-документ и открывается <main>.
    // Контроллеры добавляют только контент, затем вызывают renderFooter().
    std::string right = authBlockHtml(role);

    return std::string(R"HTML(<!doctype html>
<html lang='ru'>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ЦМРТ — реферальная программа</title>
  <link rel='icon' type='image/x-icon' href='/static/favicon.ico'>
  <link rel='stylesheet' href='/static/assets/app.css'>
</head>
<body>

<header class='hdr'>
  <div class='hdrIn'>
    <div class='hdrLeft'>
      <a class='navBtn' href='/about'>О реферальной программе</a>
      <a class='navBtn' href='/contact'>Связаться с менеджером</a>
    </div>

    <div class='hdrCenter'>
      <a class='logoWrap' href='/' aria-label='На главную'>
        <img src='/static/assets/logo.svg' alt='ЦМРТ' height='38'>
      </a>
    </div>

    <div class='hdrRight'>
)HTML") + right + R"HTML(
      <button class='burger' type='button' aria-label='Меню' onclick='toggleMenu()'>
        <span class='burgerLines'>
          <span></span><span></span><span></span>
        </span>
      </button>
    </div>
  </div>

  <nav id='menu' class='menu' aria-hidden='true'>
    <a href='/'>Главная</a>
    <a href='/request'>Записать пациента</a>
    <a href='/cabinet'>Личный кабинет</a>
    <a href='/about'>О программе</a>
    <a href='/contact'>Связаться с менеджером</a>
  </nav>
</header>

<main class='wrap'>
)HTML";
}

std::string renderFooter() {
    // Здесь закрываем <main> и документ.
    return R"HTML(
</main>

<footer class='ft'>
  <div class='ftText'>©2016-2025 Официальный сайт сети клиник ЦМРТ в Москве<br>
    ООО "ТелеРадиоМедицина" Лицензия № Л041-01148-78/00297987 ИНН 7813592589 ОГРН 1147847234040
  </div>
  <div class='ftText'>Оператор персональных данных,<br>регистрационный номер в реестре 78-23-061174</div>
  <div class='ftText'>Сайт носит информационный характер и не является публичной офертой.</div>
  <div class='ftLinks'>
    <a href='/stub'>Пользовательское соглашение</a>
    <a href='/stub'>Роспотребнадзор</a>
    <a href='/stub'>Росздравнадзор</a>
    <a href='/stub'>Оценка качества</a>
    <a href='https://cmrt.ru' target='_blank' rel='noopener'>Основной сайт</a>
  </div>
</footer>

<script>
function toggleMenu(){
  var m=document.getElementById('menu');
  if(!m) return;
  var open = (m.style.display==='block');
  m.style.display = open ? 'none' : 'block';
  m.setAttribute('aria-hidden', open ? 'true' : 'false');
}
document.addEventListener('click', function(e){
  var m=document.getElementById('menu');
  if(!m) return;
  if(m.style.display!=='block') return;
  // закрываем по клику вне меню/кнопки
  var isBurger = e.target && (e.target.closest && e.target.closest('.burger'));
  var isMenu = e.target && (e.target.closest && e.target.closest('#menu'));
  if(!isBurger && !isMenu){
    m.style.display='none';
    m.setAttribute('aria-hidden','true');
  }
});
</script>

</body>
</html>
)HTML";
}

} // namespace layout
