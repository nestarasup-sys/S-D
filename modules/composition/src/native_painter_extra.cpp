#include <studio/composition/native_painter.hpp>
#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPen>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace studio {
namespace {
void rect(QPainter &p, double x, double y, double w, double h, const QColor &c, double r = 8) {
    p.setPen(Qt::NoPen); p.setBrush(c); p.drawRoundedRect(QRectF(x,y,w,h),r,r);
}
void oval(QPainter &p, double x, double y, double w, double h, const QColor &c) {
    p.setPen(Qt::NoPen); p.setBrush(c); p.drawEllipse(QRectF(x,y,w,h));
}
void stroke(QPainter &p, std::initializer_list<QPointF> pts, const QColor &c, double w = 5) {
    if (pts.size() < 2) return;
    QPainterPath path; auto it=pts.begin(); path.moveTo(*it++);
    for (;it!=pts.end();++it) path.lineTo(*it);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c,w,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.drawPath(path);
}
void ring(QPainter &p, double x, double y, double w, double h, const QColor &c, double sw=7) {
    p.setBrush(Qt::NoBrush); p.setPen(QPen(c,sw)); p.drawEllipse(QRectF(x,y,w,h));
}
void label(QPainter &p, const QString &s, double x, double y, double w, double h, int px, QColor c) {
    QFont f("Segoe UI"); f.setPixelSize(px); f.setWeight(QFont::DemiBold);
    p.setFont(f); p.setPen(c); p.drawText(QRectF(x,y,w,h),Qt::AlignCenter,s);
}
void backdrop(QPainter &p, const std::string &id, const QColor &a, std::int64_t frame) {
    QLinearGradient g(0,0,1280,720);
    g.setColorAt(0,QColor("#08131f")); g.setColorAt(1,QColor("#1a3142"));
    p.fillRect(QRectF(0,0,1280,720),g);
    const QColor dim(a.red(),a.green(),a.blue(),70);
    const QColor bright(a.red(),a.green(),a.blue(),175);
    if (id=="night-city" || id=="financial-district") {
        oval(p,980,70,165,165,QColor("#b7d1d8"));
        for (int i=0;i<17;++i) {
            const double x=i*84-30, h=160+(i*97%290), top=550-h;
            rect(p,x,top,76,h,QColor(i%2?"#193447":"#203f51"),2);
            for(int r=0;r<7;++r) for(int c=0;c<3;++c)
                if((r+c+i)%3!=0) rect(p,x+12+c*20,top+18+r*40,9,13,i%4?dim:bright,1);
        }
        rect(p,0,552,1280,168,QColor("#0a1520"),0);
        stroke(p,{{0,591},{1280,591}},bright,2);
        if(id=="financial-district") {
            for(int i=0;i<6;++i) stroke(p,{{260.0+i*125,160},{260.0+i*125,550}},dim,2);
            stroke(p,{{200,460},{430,390},{690,423},{930,275},{1100,185}},a,9);
        }
    } else if(id=="server-room") {
        for(int i=0;i<5;++i) {
            double x=52+i*254; rect(p,x,65,216,555,QColor("#102635"),13);
            rect(p,x+13,80,190,518,QColor("#192e3d"),7);
            for(int j=0;j<10;++j) {
                rect(p,x+29,103+j*47,160,33,QColor("#071820"),5);
                oval(p,x+145,113+j*47,12,12,j%3?bright:QColor("#ffbe75"));
                stroke(p,{{x+43,120.0+j*47},{x+120,120.0+j*47}},dim,3);
            }
        }
        rect(p,0,633,1280,87,QColor("#09121b"),0);
    } else if(id=="newsroom") {
        rect(p,80,88,1120,460,QColor("#162b3b"),14);
        for(int i=0;i<3;++i) {
            double x=112+i*353; rect(p,x,125,324,260,QColor("#0c1b27"),8);
            for(int j=0;j<5;++j) rect(p,x+30,158+j*33,235-(j%3)*46,8,j==0?a:QColor("#466171"),3);
        }
        rect(p,0,542,1280,178,QColor("#0c1723"),0);
        rect(p,225,475,830,85,QColor("#294052"),12);
        label(p,"BREAKING   /   DOCUMENTARY",300,486,680,57,29,QColor("#e0ebf0"));
    } else if(id=="laboratory-scene") {
        for(int i=0;i<5;++i) stroke(p,{{i*320.0,0},{640.0+(i-2)*65,480}},dim,4);
        rect(p,145,116,990,395,QColor("#173142"),14);
        rect(p,185,151,410,280,QColor("#0c2030"),8);
        for(int i=0;i<3;++i) ring(p,230+i*105,213,70,70,dim,4);
        stroke(p,{{680,345},{750,230},{820,335},{890,215},{990,330}},a,10);
        rect(p,0,530,1280,190,QColor("#102431"),0);
        for(int i=0;i<6;++i) {rect(p,200+i*150,493,65,65,QColor("#b0c1ca"),7);oval(p,213+i*150,483,38,19,a);}
    } else if(id=="map-room") {
        for(int i=0;i<10;++i) stroke(p,{{i*140.0,0},{i*140.0,720}},dim,2);
        for(int i=0;i<7;++i) stroke(p,{{0,i*120.0},{1280,i*120.0}},dim,2);
        QPainterPath land; land.moveTo(128,180); land.lineTo(295,122); land.lineTo(432,210);
        land.lineTo(385,364); land.lineTo(265,430); land.lineTo(156,311); land.closeSubpath();
        p.fillPath(land,QColor("#214958"));
        land=QPainterPath(); land.moveTo(655,134); land.lineTo(818,165); land.lineTo(940,295);
        land.lineTo(841,419); land.lineTo(690,349); land.closeSubpath(); p.fillPath(land,QColor("#214958"));
        stroke(p,{{270,280},{520,210},{774,301},{1040,180}},a,8);
        for(const auto &pt:{QPointF(270,280),QPointF(774,301),QPointF(1040,180)}) {oval(p,pt.x()-17,pt.y()-17,34,34,a);oval(p,pt.x()-6,pt.y()-6,12,12,QColor("#061520"));}
    } else if(id=="cyber-grid") {
        for(int i=0;i<23;++i) stroke(p,{{i*70.0,0},{640.0+(i-11)*155,720}},dim,2);
        for(int i=0;i<12;++i) stroke(p,{{0,260+i*i*4.0},{1280,260+i*i*4.0}},dim,2);
        oval(p,415,112,450,450,QColor(a.red(),a.green(),a.blue(),23));
        ring(p,447,144,386,386,a,7);ring(p,525,222,230,230,dim,4);
        rect(p,595,283,90,110,QColor("#08141f"),12);stroke(p,{{615,325},{640,348},{671,299}},a,11);
    } else if(id=="archive-room") {
        for(int i=0;i<6;++i) {
            double x=30+i*215;
            rect(p,x,55,190,530,QColor("#223444"),9);
            for(int j=0;j<6;++j) {
                rect(p,x+12,80+j*76,164,59,QColor("#344b58"),5);
                rect(p,x+59,98+j*76,70,10,a,3);
                rect(p,x+150,100+j*76,10,10,QColor("#c6d2d9"),2);
            }
        }
        rect(p,0,590,1280,130,QColor("#0b1925"),0);
    }
    Q_UNUSED(frame);
}
} // namespace

// Procedural vector illustrations: no external asset, download or personal assetId required.
// Drawn through the same QPainter composition pipeline as all existing native objects.
bool paintExtendedNative(QPainter &p, const Node &n, std::int64_t frame) {
    const std::string &id=n.nativeId;
    static const std::initializer_list<const char*> scenes={
        "night-city","server-room","newsroom","laboratory-scene",
        "map-room","financial-district","cyber-grid","archive-room"
    };
    const bool isScene=std::find_if(scenes.begin(),scenes.end(),[&](const char *s){return id==s;})!=scenes.end();
    static const std::initializer_list<const char*> objects={
        "laptop","tablet","camera","microphone","headphones","chip","server","cloud",
        "wifi","lock","notification","robot","chat-bubble","email","newspaper","book",
        "calendar","clock","hourglass","eye","magnifier","target","chart-line","chart-pie",
        "chart-donut","chart-area","timeline","flowchart","network","dashboard",
        "progress-ring","data-table","bank","wallet","credit-card","cash","receipt",
        "shopping-cart","factory","briefcase","atom","dna","flask","brain","globe",
        "map-pin","compass","rocket","satellite","lightbulb"
    };
    if(!isScene && std::find_if(objects.begin(),objects.end(),[&](const char *s){return id==s;})==objects.end())
        return false;
    const QColor a(QString::fromStdString(n.color));
    const QColor accent=a.isValid()?a:QColor("#55e2c4");
    const QColor white("#eef5f8"), secondary("#7598a8"), shade("#203747"), dark("#0e2230");
    p.save();
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setClipRect(QRectF(0,0,n.width,n.height),Qt::IntersectClip);
    if(isScene) {
        const double scale=std::max(n.width/1280.0,n.height/720.0);
        p.translate((n.width-1280*scale)/2,(n.height-720*scale)/2);
        p.scale(scale,scale); backdrop(p,id,accent,frame);p.restore();return true;
    }
    p.scale(n.width/320.0,n.height/240.0);
    // Small internal glow gives icons readable edges on dark documentary scenes.
    oval(p,45,13,230,215,QColor(accent.red(),accent.green(),accent.blue(),17));

    if(id=="laptop") {
        rect(p,52,35,216,151,secondary,11);rect(p,62,45,196,129,dark,5);
        stroke(p,{{83,75},{165,75},{198,109},{235,109}},accent,7);
        rect(p,30,188,260,14,white,4);rect(p,133,188,54,6,secondary,2);
    } else if(id=="tablet") {
        rect(p,91,11,138,218,secondary,17);rect(p,100,25,120,185,dark,9);
        ring(p,143,94,34,34,accent,5);stroke(p,{{119,155},{201,155}},accent,7);oval(p,155,216,10,10,white);
    } else if(id=="camera") {
        rect(p,55,70,210,126,secondary,16);rect(p,84,55,75,20,accent,5);
        ring(p,125,91,74,74,dark,18);ring(p,137,103,50,50,accent,5);
        oval(p,233,89,14,14,white);rect(p,209,56,37,15,white,4);
    } else if(id=="microphone") {
        rect(p,125,25,70,127,accent,31);stroke(p,{{101,113},{101,157},{120,178},{159,188},{198,178},{219,155},{219,113}},white,9);
        stroke(p,{{160,189},{160,220}},secondary,9);stroke(p,{{110,223},{210,223}},secondary,9);
        for(int i=0;i<4;++i)stroke(p,{{137,57.0+i*21},{183,57.0+i*21}},dark,2);
    } else if(id=="headphones") {
        p.setBrush(Qt::NoBrush);p.setPen(QPen(white,16));p.drawArc(QRectF(79,23,162,175),0,180*16);
        rect(p,66,111,42,86,accent,11);rect(p,212,111,42,86,accent,11);
        rect(p,92,128,14,50,secondary,4);rect(p,214,128,14,50,secondary,4);
    } else if(id=="chip") {
        rect(p,85,45,150,150,accent,17);rect(p,105,65,110,110,dark,9);
        for(int i=0;i<5;++i){stroke(p,{{113.0+i*23,24},{113.0+i*23,45}},secondary,8);stroke(p,{{113.0+i*23,195},{113.0+i*23,218}},secondary,8);}
        for(int i=0;i<5;++i){stroke(p,{{62,73.0+i*24},{85,73.0+i*24}},secondary,8);stroke(p,{{235,73.0+i*24},{258,73.0+i*24}},secondary,8);}
        label(p,"AI",119,91,82,58,52,white);
    } else if(id=="server") {
        for(int i=0;i<3;++i){rect(p,60,32+i*65,200,54,shade,10);rect(p,73,43+i*65,20,20,accent,5);
            stroke(p,{{117,59.0+i*65},{223,59.0+i*65}},secondary,5);oval(p,234,48+i*65,12,12,white);}
    } else if(id=="cloud") {
        oval(p,65,92,108,104,accent);oval(p,119,42,122,145,accent);oval(p,181,96,82,91,accent);
        rect(p,72,130,190,57,accent,11);stroke(p,{{161,104},{161,197}},dark,7);
        stroke(p,{{138,172},{161,195},{184,172}},dark,7);
    } else if(id=="wifi") {
        for(int i=0;i<3;++i) {int x=52+i*37; p.setPen(QPen(i==0?secondary:accent,9,Qt::SolidLine,Qt::RoundCap));
            p.setBrush(Qt::NoBrush);p.drawArc(QRectF(x,49+i*40,216-i*74,172-i*80),35*16,110*16);}
        oval(p,148,185,24,24,white);
    } else if(id=="lock") {
        p.setPen(QPen(secondary,17));p.setBrush(Qt::NoBrush);p.drawArc(QRectF(100,24,120,156),0,180*16);
        rect(p,78,113,164,106,accent,13);oval(p,150,145,20,20,dark);rect(p,157,160,6,30,dark,2);
    } else if(id=="notification") {
        p.setBrush(accent);p.setPen(Qt::NoPen);p.drawPie(QRectF(86,40,148,160),0,180*16);
        rect(p,93,119,134,62,accent,5);oval(p,147,183,26,19,white);oval(p,222,28,47,47,QColor("#ff817a"));
    } else if(id=="robot") {
        rect(p,76,68,168,120,secondary,25);rect(p,98,88,124,77,dark,12);
        oval(p,121,110,22,22,accent);oval(p,178,110,22,22,accent);
        stroke(p,{{130,150},{190,150}},white,5);stroke(p,{{160,68},{160,44}},accent,7);oval(p,151,30,18,18,accent);
        rect(p,49,105,28,48,accent,7);rect(p,243,105,28,48,accent,7);
    } else if(id=="chat-bubble") {
        rect(p,47,38,226,152,accent,26);
        QPainterPath tail;tail.moveTo(78,181);tail.lineTo(78,224);tail.lineTo(132,184);tail.closeSubpath();p.fillPath(tail,accent);
        for(int i=0;i<3;++i)oval(p,96+i*54,98,25,25,dark);
    } else if(id=="email") {
        rect(p,39,44,242,157,shade,12);
        stroke(p,{{46,55},{160,141},{275,55}},accent,10);
        stroke(p,{{48,188},{123,128}},white,5);stroke(p,{{273,188},{197,128}},white,5);
    } else if(id=="newspaper") {
        rect(p,52,28,216,192,QColor("#dbe7eb"),7);
        rect(p,68,44,184,32,shade,3);label(p,"NEWS",75,44,171,29,26,white);
        rect(p,69,91,77,94,accent,3);
        for(int i=0;i<5;++i)rect(p,160,92+i*19,86-(i%2)*16,6,secondary,2);
        rect(p,69,200,179,5,secondary,2);
    } else if(id=="book") {
        QPainterPath pg;pg.moveTo(160,60);pg.quadTo(116,30,55,49);pg.lineTo(55,195);
        pg.quadTo(115,177,160,207);pg.quadTo(212,177,265,195);pg.lineTo(265,49);pg.quadTo(205,30,160,60);
        p.fillPath(pg,accent);stroke(p,{{160,58},{160,208}},dark,5);
        for(int i=0;i<4;++i){stroke(p,{{76,79.0+i*25},{135,79.0+i*25}},dark,4);stroke(p,{{183,79.0+i*25},{243,79.0+i*25}},dark,4);}
    } else if(id=="calendar") {
        rect(p,67,35,186,182,white,12);rect(p,67,35,186,54,accent,12);
        stroke(p,{{109,23},{109,62}},shade,12);stroke(p,{{211,23},{211,62}},shade,12);
        for(int i=0;i<3;++i)for(int j=0;j<4;++j)oval(p,93+j*36,108+i*32,14,14,(i==1&&j==2)?accent:secondary);
    } else if(id=="clock") {
        ring(p,74,24,172,172,accent,12);oval(p,153,102,14,14,white);
        stroke(p,{{160,110},{160,55}},white,10);stroke(p,{{160,110},{205,138}},white,10);
    } else if(id=="hourglass") {
        stroke(p,{{84,27},{236,27}},white,12);stroke(p,{{84,213},{236,213}},white,12);
        stroke(p,{{104,35},{104,71},{157,120},{104,171},{104,206}},accent,9);
        stroke(p,{{216,35},{216,71},{163,120},{216,171},{216,206}},accent,9);
        oval(p,139,174,42,27,accent);
    } else if(id=="eye") {
        QPainterPath path;path.moveTo(26,120);path.quadTo(160,-20,294,120);path.quadTo(160,260,26,120);
        p.setBrush(shade);p.setPen(QPen(accent,8));p.drawPath(path);
        oval(p,108,68,104,104,accent);oval(p,138,96,44,44,dark);oval(p,148,97,18,18,white);
    } else if(id=="magnifier") {
        ring(p,73,34,136,136,accent,17);stroke(p,{{194,164},{260,219}},white,20);
        stroke(p,{{106,83},{127,62}},white,5);
    } else if(id=="target") {
        ring(p,63,26,190,190,secondary,9);ring(p,100,63,116,116,accent,10);
        oval(p,141,104,37,37,accent);stroke(p,{{170,117},{277,41}},white,8);
    } else if(id=="chart-line" || id=="chart-area") {
        stroke(p,{{45,31},{45,211},{285,211}},secondary,5);
        if(id=="chart-area") {QPainterPath fill;fill.moveTo(48,205);fill.lineTo(88,151);fill.lineTo(137,173);fill.lineTo(184,94);fill.lineTo(235,117);fill.lineTo(270,45);fill.lineTo(270,206);fill.closeSubpath();p.fillPath(fill,QColor(accent.red(),accent.green(),accent.blue(),88));}
        stroke(p,{{48,192},{88,151},{137,173},{184,94},{235,117},{270,45}},accent,10);
        for(const auto &pt:{QPointF(88,151),QPointF(184,94),QPointF(270,45)})oval(p,pt.x()-8,pt.y()-8,16,16,white);
    } else if(id=="chart-pie" || id=="chart-donut") {
        p.setPen(Qt::NoPen);p.setBrush(accent);p.drawPie(QRectF(72,30,175,175),25*16,275*16);
        p.setBrush(secondary);p.drawPie(QRectF(72,30,175,175),300*16,60*16);
        if(id=="chart-donut")oval(p,126,84,67,67,dark);
    } else if(id=="timeline") {
        stroke(p,{{43,123},{277,123}},secondary,7);
        for(int i=0;i<4;++i){double x=55+i*70.0;oval(p,x-12,111,24,24,i==2?white:accent);
            rect(p,x-23,i%2?142:60,46,40,shade,7);stroke(p,{{x, i%2?135.0:100.0},{x,i%2?142.0:100.0}},accent,3);}
    } else if(id=="flowchart") {
        rect(p,113,20,94,46,accent,7);rect(p,28,163,112,51,shade,6);rect(p,180,163,112,51,shade,6);
        stroke(p,{{160,66},{160,111},{85,111},{85,163}},white,5);stroke(p,{{160,111},{236,111},{236,163}},white,5);
    } else if(id=="network") {
        const QPointF pts[]={QPointF(160,33),QPointF(68,101),QPointF(250,101),QPointF(102,195),QPointF(222,195)};
        for(int i=0;i<5;++i)for(int j=i+1;j<5;++j)if((i+j)%2)stroke(p,{pts[i],pts[j]},secondary,3);
        for(int i=0;i<5;++i)oval(p,pts[i].x()-17,pts[i].y()-17,34,34,i%2?accent:white);
    } else if(id=="dashboard") {
        rect(p,30,31,260,187,shade,10);rect(p,45,47,111,63,accent,7);
        rect(p,169,47,105,63,dark,7);ring(p,197,62,50,37,accent,6);
        for(int i=0;i<5;++i)rect(p,50+i*39,191-(i%3)*22,27,12+(i%3)*22,accent,4);
        stroke(p,{{176,131},{258,131}},secondary,5);stroke(p,{{176,152},{258,152}},secondary,5);
    } else if(id=="progress-ring") {
        ring(p,78,29,164,164,shade,18);
        p.setBrush(Qt::NoBrush);p.setPen(QPen(accent,18,Qt::SolidLine,Qt::RoundCap));p.drawArc(QRectF(78,29,164,164),90*16,245*16);
        label(p,"75%",109,83,104,60,37,white);
    } else if(id=="data-table") {
        rect(p,33,31,254,184,shade,9);rect(p,44,43,232,35,accent,4);
        for(int i=1;i<5;++i)stroke(p,{{43,78.0+i*27},{276,78.0+i*27}},secondary,2);
        for(int i=1;i<4;++i)stroke(p,{{44.0+i*58,78},{44.0+i*58,211}},secondary,2);
    } else if(id=="bank") {
        QPainterPath roof;roof.moveTo(35,86);roof.lineTo(160,20);roof.lineTo(285,86);roof.closeSubpath();p.fillPath(roof,accent);
        for(int i=0;i<4;++i){rect(p,56+i*60,101,26,101,white,4);rect(p,48+i*60,91,42,13,shade,3);}
        rect(p,32,207,256,14,accent,4);
    } else if(id=="wallet") {
        rect(p,35,52,248,145,QColor("#a37d57"),14);rect(p,156,103,127,64,accent,10);
        oval(p,175,123,25,25,dark);stroke(p,{{55,82},{240,82}},white,5);
    } else if(id=="credit-card") {
        rect(p,39,48,242,152,accent,15);rect(p,39,78,242,30,dark,0);
        rect(p,66,131,52,37,QColor("#f4d28e"),6);rect(p,174,153,73,9,white,4);
    } else if(id=="cash") {
        rect(p,33,63,242,126,accent,9);rect(p,45,73,242,126,secondary,9);
        ring(p,112,86,96,96,white,5);label(p,"$",125,96,74,75,62,white);
    } else if(id=="receipt") {
        QPainterPath paper;paper.moveTo(75,22);paper.lineTo(245,22);paper.lineTo(245,223);paper.lineTo(222,210);
        paper.lineTo(196,223);paper.lineTo(170,210);paper.lineTo(142,223);paper.lineTo(114,210);paper.lineTo(75,223);paper.closeSubpath();
        p.fillPath(paper,white);rect(p,95,51,130,15,accent,3);
        for(int i=0;i<5;++i)rect(p,95,91+i*22,100-(i%2)*20,7,secondary,3);
    } else if(id=="shopping-cart") {
        stroke(p,{{39,55},{67,55},{105,161},{248,161},{274,83},{87,83}},accent,12);
        oval(p,117,184,27,27,white);oval(p,218,184,27,27,white);
    } else if(id=="factory") {
        QPainterPath factory;factory.moveTo(42,111);factory.lineTo(105,75);factory.lineTo(105,111);
        factory.lineTo(168,75);factory.lineTo(168,111);factory.lineTo(232,75);factory.lineTo(232,111);
        factory.lineTo(278,111);factory.lineTo(278,206);factory.lineTo(42,206);factory.closeSubpath();
        p.fillPath(factory,shade);rect(p,55,27,32,81,secondary,3);
        for(int i=0;i<4;++i)rect(p,73+i*52,137,28,34,accent,3);
    } else if(id=="briefcase") {
        rect(p,41,82,238,128,shade,11);p.setBrush(Qt::NoBrush);p.setPen(QPen(accent,11));p.drawRoundedRect(QRectF(118,38,84,70),10,10);
        rect(p,41,115,238,43,accent,4);rect(p,144,124,32,40,white,4);
    } else if(id=="atom") {
        for(int i=0;i<3;++i) {p.save();p.translate(160,120);p.rotate(i*60);ring(p,-111,-47,222,94,accent,5);p.restore();}
        oval(p,145,105,30,30,white);
    } else if(id=="dna") {
        for(int i=0;i<=15;++i) {double y=21+i*13;double x=160+60*std::sin(i*.52);
            double right=320-x;stroke(p,{{x,y},{right,y}},secondary,4);}
        for(int j=0;j<2;++j){QPainterPath path;for(int i=0;i<=65;++i){double y=18+i*3.15;double x=160+(j? -1:1)*60*std::sin(i*.12);
            if(i==0)path.moveTo(x,y);else path.lineTo(x,y);}
            p.setPen(QPen(j?white:accent,8,Qt::SolidLine,Qt::RoundCap));p.drawPath(path);}
    } else if(id=="flask") {
        stroke(p,{{131,21},{187,21}},white,9);stroke(p,{{143,23},{143,91},{83,204},{237,204},{177,91},{177,23}},white,10);
        QPainterPath liquid;liquid.moveTo(115,148);liquid.quadTo(160,164,205,148);liquid.lineTo(232,200);liquid.lineTo(88,200);liquid.closeSubpath();p.fillPath(liquid,accent);
        oval(p,143,172,15,15,white);oval(p,178,180,10,10,white);
    } else if(id=="brain") {
        for(const auto &pt:{QPointF(105,74),QPointF(158,50),QPointF(210,75),QPointF(101,132),QPointF(160,135),QPointF(215,130)})
            oval(p,pt.x()-34,pt.y()-34,68,68,accent);
        stroke(p,{{160,28},{160,199}},dark,5);stroke(p,{{110,92},{138,120},{111,152}},dark,4);
        stroke(p,{{209,82},{181,115},{205,152}},dark,4);
    } else if(id=="globe") {
        ring(p,77,29,166,166,accent,9);ring(p,124,29,72,166,secondary,4);
        stroke(p,{{82,91},{237,91}},secondary,4);stroke(p,{{82,140},{237,140}},secondary,4);
        stroke(p,{{160,30},{160,194}},secondary,3);
    } else if(id=="map-pin") {
        QPainterPath pin;pin.moveTo(160,220);pin.cubicTo(95,149,81,122,81,93);pin.cubicTo(81,49,114,19,160,19);
        pin.cubicTo(206,19,239,49,239,93);pin.cubicTo(239,122,225,149,160,220);
        p.fillPath(pin,accent);oval(p,125,59,70,70,dark);
    } else if(id=="compass") {
        ring(p,74,24,172,172,secondary,8);
        QPainterPath arrow;arrow.moveTo(194,69);arrow.lineTo(172,155);arrow.lineTo(119,177);arrow.lineTo(144,97);arrow.closeSubpath();p.fillPath(arrow,accent);
        oval(p,155,111,12,12,white);
    } else if(id=="rocket") {
        p.save();p.translate(160,125);p.rotate(35);
        QPainterPath body;body.moveTo(-37,80);body.lineTo(-37,-47);body.quadTo(-32,-89,0,-105);
        body.quadTo(32,-89,37,-47);body.lineTo(37,80);body.closeSubpath();p.fillPath(body,white);
        oval(p,-17,-56,34,34,accent);stroke(p,{{-40,21},{-73,75}},accent,16);stroke(p,{{40,21},{73,75}},accent,16);
        QPainterPath flame;flame.moveTo(-22,81);flame.quadTo(0,161,22,81);flame.closeSubpath();p.fillPath(flame,QColor("#ffad64"));
        p.restore();
    } else if(id=="satellite") {
        rect(p,123,86,74,68,accent,9);rect(p,16,67,102,105,shade,3);rect(p,202,67,102,105,shade,3);
        for(int i=0;i<3;++i)for(int j=0;j<3;++j){rect(p,24+i*29,74+j*30,24,24,secondary,1);rect(p,210+i*29,74+j*30,24,24,secondary,1);}
        stroke(p,{{160,86},{160,38}},white,6);oval(p,153,30,14,14,white);
    } else if(id=="lightbulb") {
        oval(p,95,24,130,137,accent);rect(p,124,145,72,48,secondary,9);
        rect(p,132,201,57,14,white,5);stroke(p,{{133,112},{155,135},{181,112}},dark,6);
        for(int i=0;i<3;++i)stroke(p,{{134,158.0+i*14},{187,158.0+i*14}},dark,3);
    }
    p.restore();
    return true;
}
} // namespace studio
