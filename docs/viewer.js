"use strict";
const viewer = document.getElementById("viewer");
const projectSelect = document.getElementById("project");
const repoUrl = "https://github.com/yuxuan-518/atmega2560-embedded-foundations";
const words = {
  en: { title:"ATmega2560 · Project overview", intro:"Scroll inside the window to explore all 40 project cards. Click a card to view its code.", jump:"Jump to", pdf:"English PDF", text:"Text version", region:"Scrollable project cards", note:"Projects 20 and 23 are hardware exercises without separate source directories.", outcome:"What the project does", technical:"Technical implementation and concepts" },
  zh: { title:"ATmega2560 · 项目总览", intro:"在下方窗口内向下滚动，查看全部 40 张项目卡；点击卡片可以打开对应代码。", jump:"跳转项目", pdf:"中文 PDF", text:"文字版", region:"可滚动的项目卡预览窗口", note:"项目 20 和 23 为硬件实验，没有独立的代码目录。", outcome:"呈现效果", technical:"技术实现与新知识" }
};
let language = new URLSearchParams(location.search).get("lang") === "zh" ? "zh" : "en";
let currentId = Math.min(40, Math.max(1, Number(location.hash.replace("#project-", "")) || 1));
let ticking = false;

function jump(id, behavior = "auto") {
  const card = document.getElementById(`project-${id}`);
  if (!card) return;
  viewer.scrollTo({top:Math.max(0,card.offsetTop - parseFloat(getComputedStyle(viewer).paddingTop)),behavior});
  currentId = id;
  projectSelect.value = String(id);
  document.getElementById("position").textContent = `${String(id).padStart(2,"0")} / 40`;
}

function render() {
  const copy = words[language];
  document.documentElement.lang = language === "zh" ? "zh-CN" : "en";
  document.title = copy.title;
  for (const [id,key] of [["heading","title"],["intro","intro"],["jump-label","jump"],["pdf","pdf"],["text-version","text"],["note","note"]]) document.getElementById(id).textContent = copy[key];
  viewer.setAttribute("aria-label", copy.region);
  document.getElementById("pdf").href = `project-cards/ATmega2560_Project_Cards_${language === "zh" ? "Chinese" : "English"}.pdf`;
  document.getElementById("text-version").href = `${repoUrl}/blob/main/docs/project-cards/${language === "zh" ? "project-cards.zh-CN.md" : "project-cards.md"}`;
  for (const lang of ["en","zh"]) {
    const a = document.getElementById(`lang-${lang}`);
    if (language === lang) a.setAttribute("aria-current","page"); else a.removeAttribute("aria-current");
  }
  viewer.replaceChildren();
  projectSelect.replaceChildren();
  const fragment = document.createDocumentFragment();
  for (const card of projectCards[language]) {
    const label = `${String(card.id).padStart(2,"0")} ${card.title}`;
    const option = new Option(label, String(card.id));
    projectSelect.add(option);
    const figure = document.createElement("figure");
    figure.className = "card";
    figure.id = `project-${card.id}`;
    figure.dataset.project = String(card.id);
    const img = document.createElement("img");
    img.src = card.image;
    img.alt = label;
    img.width = card.width;
    img.height = card.height;
    img.loading = card.id < 3 || card.id === currentId ? "eager" : "lazy";
    img.decoding = "async";
    if (card.url) {
      const a = document.createElement("a");
      a.href = card.url;
      a.target = "_blank";
      a.rel = "noopener";
      a.append(img);
      figure.append(a);
    } else figure.append(img);
    const caption = document.createElement("figcaption");
    caption.className = "sr-only";
    caption.textContent = `${copy.outcome}: ${card.effect} ${copy.technical}: ${card.technical} ${card.note || ""}`;
    figure.append(caption);
    fragment.append(figure);
  }
  viewer.append(fragment);
  const url = new URL(location.href);
  url.searchParams.set("lang",language);
  history.replaceState(null,"",url);
  requestAnimationFrame(() => jump(currentId));
}

projectSelect.addEventListener("change", () => {
  jump(Number(projectSelect.value));
  history.replaceState(null,"",`?lang=${language}#project-${currentId}`);
});
for (const lang of ["en","zh"]) document.getElementById(`lang-${lang}`).addEventListener("click", event => {
  event.preventDefault();
  language = lang;
  render();
});
viewer.addEventListener("scroll", () => {
  if (ticking) return;
  ticking = true;
  requestAnimationFrame(() => {
    const top = viewer.scrollTop;
    const cards = [...viewer.children];
    const current = cards.find(c => c.offsetTop + c.offsetHeight > top + 35) || cards.at(-1);
    if (current) {
      currentId = Number(current.dataset.project);
      projectSelect.value = String(currentId);
      document.getElementById("position").textContent = `${String(currentId).padStart(2,"0")} / 40`;
    }
    ticking = false;
  });
}, {passive:true});
render();
