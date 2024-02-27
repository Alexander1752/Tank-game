# Tank game

Programul porneste de la framework-ul de grafica UPB [1] si implementează toate cerințele punctate din enunțul temei [2], atât cele de bază, cât și cele avansate, la care se adaugă următoarele:

- tancurile inamice se deplasează către jucător în mod rudimentar (în continuare iau decizii aleatoare, inclusiv de a se depărta de jucător, dar au o preferință puternică de a se apropia de acesta)
- scorul și timpul rămas din joc sunt afișate pe ecran; când tancul jucătorului este distrus sau când timpul de joc s-au scurs, culoarea textului se schimbă în roșu și controalele jucătorului sunt dezactivate
- coliziuni cu clădiri nealiniate cu axele - suplimentar, testarea coliziunilor ce implică tancuri se realizează prin testarea coliziunii cu fiecare dintre elementele lor componente (astfel, de exemplu, tunul tancului nu poate să "intre" în clădiri/alte tancuri, respectiv un tanc lovit de proiectil în tun este avariat)
- efect de knockback la coliziune: un tanc ce se ciocnește de alt tanc/clădire este împins cu o viteză ce variază polinomial cu timpul înapoi, timp în care acesta nu se mai poate mișca din proprie inițiativă (este "stunned"), valabil și pentru jucător; knockback-ul ia în calcul într-o formă rudimentară și viteza obiectelor ce se ciocnesc
- jucătorul poate deplasa camera mai aproape/departe de tancul propriu folosind săgețile sus-jos, respectiv să modifice FOV-ul folosind săgețile stânga-dreapta
- minimap ortografic ce se poate redimensiona (folosind tastele 4-6 de pe numpad)
- indicator de cooldown al tunului: sub minimap apare o bară solidă roșie ce scade în lungime odată cu scăderea timpului până se poate trage din nou
- match start timer: la începutul jocului, niciun tanc nu poate trage timp de 5 secunde, pentru a oferi jucătorului ocazia de a se familiariza cu harta generată aleator
- iluminarea Phong este implementată în fragment shader, așa cum este descrisă în laboratorul 8 [3]

 [1] https://github.com/UPB-Graphics/gfx-framework
 [2] https://ocw.cs.pub.ro/courses/egc/teme/2023/02
 [3] https://ocw.cs.pub.ro/courses/egc/laboratoare/08
