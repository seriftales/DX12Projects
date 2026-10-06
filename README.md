# DirectX 12 3D Graphics Engine & Architecture Projects

Bu depo, DirectX 12 API'si kullanılarak C++ ile geliştirilmiş; **Çarpışma Fiziği (Collision Detection)**, **Prosedürel Geometri (Procedural Generation)** ve **Veri/Bellek Optimizasyonu** konularını ele alan üç farklı projeyi tek bir Monorepo altında toplamaktadır.

> **⚠️ Akademik Şablon ve Katkı Beyanı (Disclaimer)**
> Bu projeler, bilgisayar grafikleri dersi kapsamında sağlanan temel Direct3D 12 mimari şablonu üzerine inşa edilmiştir. Bu şablon; pencere oluşturma, temel device başlatma ve komut kuyruğutahsisleri gibi standart altyapı işlemlerini üstlenmektedir. 
> ** Mimari Katkılarım:** Kesişim algoritmalarının matematiksel tasarımı, prosedürel mesh üretim mantığı, bellek optimizasyonları , PSO durum değişimleri ve çalışma zamanı ) güvenlik önlemlerinin tamamı tarafımca C++ standartlarına uygun olarak sisteme entegre edilmiştir.

## Proje 1: 3D Box-to-Box Intersection 

Uzayda doğrusal olarak hareket ettirilen 3 boyutlu iki kutunun fiziksel kesişimlerini hesaplayan ve bu kesişim hacmini gerçek zamanlı olarak ekranda yeni bir katı obje şeklinde görselleştiren motor modülüdür.

* **Kesişim Algoritmaları:** Möller-Trumbore ve Point-in-OBB algoritmaları ile CPU üzerinde kenar-yüzey ışın testleri ve Ters Matris dönüşümleriyle kutu içi nokta algılama işlemleri yapılmıştır.
* **Dinamik Hacim İnşası:** Kesişim sonucunda elde edilen dağınık nokta bulutunun uzaydaki Min/Max sınırları hesaplanmış, bu değerlerden anlık bir Ölçekleme (Scale) ve Öteleme (Translation) matrisi sentezlenerek referans model kesişim merkezine oturtulmuştur.
* **PSO (Pipeline State Object) Yönetimi:** Kesişim hacmini net bir şekilde görebilmek adına, donanımsal PSO değişimleri uygulanmıştır. Çizim döngüsünde dış kutular için Wireframe, içteki kesişim hacmi için ise Katı (Solid/Phong) render kuralları anlık olarak değiştirilir.

## Proje 2:Sphere Generation

Dışarıdan statik `.obj` dosyaları yüklemek yerine, Küresel Koordinat trigonometrisi kullanılarak çalışma zamanında  sıfırdan 3 boyutlu küreinşa eden algoritmik geometri projesidir.

* **Dinamik LOD :** Kullanıcı girdisiyle Theta (enlem) ve Phi (boylam) parça sayıları program çalışırken asenkron olarak değiştirilerek, objenin poligon çözünürlüğü gerçek zamanlı artırılıp azaltılabilmektedir.
* **Donanım Optimizasyonu:** Yüzey normallerini bulmak için CPU'yu yoran Cross Product operasyonları yerine, "Birim Küre" kuralı gereği noktaların merkezden uzaklık vektörleri doğrudan Normal vektörü olarak atanmıştır.
* **Bellek Taşırma Güvenliği :** Dinamik üçgen üretiminde kullanıcı girdisi kaynaklı VRAM taşırmalarını (Buffer Overflow) ve bellek erişim ihlallerini engellemek için katı `MAX_ALLOWED_VERTICES` sınırlandırmaları sisteme entegre edilmiştir.
Not:Klavyeden 1-8 arası rakamlar tuşlanarak diğer şekiller de oluşturulabilir.

## Proje 3: HeightMap for Stairs

Oyun motorlarında karakterlerin engebeli arazilerde veya merdivenlerde yürümesini sağlayan "Arazi Takibi" mekaniğinin, CPU/GPU donanım kaynaklarını tüketmeden nasıl tasarlanacağını gösteren bir bellek optimizasyonu projesidir.

* **Baking :** Oyun döngüsü başlamadan önce, sahnedeki merdiven modelinin üzerinden 120x120 boyutlarında ızgara şeklinde sanal ışınlar atılmış, elde edilen statik çarpışma verileri bir Yükseklik Haritası (HeightMap) olarak 2 boyutlu bir RAM matrisine kaydedilmiştir.
* **$O(1)$ Çalışma Zamanı Karmaşıklığı:** Oyun esnasında saniyede 60 kere ağır donanımsal ışın testleri yapmak yerine, kameranın gerçek dünya (X, Z) koordinatları matematiksel olarak matris indeksine dönüştürülmüş ve ilgili Y (yükseklik) değeri doğrudan bellekten sabit zamanlı $O(1)$ olarak okunmuştur.
* **Güvenli Dizi Erişimi:** Karakterin dünya sınırlarının dışına çıkarak C++ "Out of Bounds" bellek hatalarına sebep olmasını önlemek adına, matris indeks hesaplamaları bellek adresleme aşamasında katı bir şekilde sınırlandırılmıştır.

## Derleme ve Çalıştırma (Build Instructions)

Bu depo Visual Studio 2022 çözüm mimarisine (`.sln`) uygun olarak yapılandırılmıştır.

1. Depoyu yerel makinenize klonlayın.
2. Çözüm dosyasını (`.sln`) Visual Studio 2022 ile açın.
3. Derleme hedefini `x64` ve modunu `Debug` veya `Release` olarak seçin.
4. İncelemek istediğiniz projeye Solution Explorer üzerinden sağ tıklayıp **"Set as Startup Project" (Başlangıç Projesi Olarak Ayarla)** seçeneğini işaretleyin.
5. F5 (Local Windows Debugger) ile projeyi derleyip çalıştırın.
