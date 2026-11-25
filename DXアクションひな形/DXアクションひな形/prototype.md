# 2D 平台跳躍遊戲 Prototype 底層邏輯架構

本文件定義遊戲原型（Prototype）的核心 C++ 類別架構，專注於角色控制、平台物件、檢查點觸發以及無縫關卡切換的底層邏輯。

---

## 1. 平台物件的基礎類別 (Platform)

所有地圖上的靜態物件（包含手繪物件的碰撞體）皆繼承自此基礎類別。主要提供物理碰撞的邊界與渲染基礎。

```cpp
// 基礎碰撞體結構
struct BoundingBox {
    float x, y;
    float width, height;
    
    // 基礎的 AABB 碰撞檢測邏輯
    bool Intersects(const BoundingBox& other) const {
        return (x < other.x + other.width && x + width > other.x &&
                y < other.y + other.height && y + height > other.y);
    }
};

// 平台基礎類別
class Platform {
protected:
    BoundingBox collider;
    // 視覺素材的指標 (例如 DirectX 的 Texture 或 Sprite)
    void* textureData; 

public:
    Platform(float x, float y, float w, float h) {
        collider = {x, y, w, h};
    }
    virtual ~Platform() = default;

    virtual void Update(float deltaTime) {} // 預留給動態平台
    virtual void Render() const = 0;        // 交由底層圖形 API 實作

    const BoundingBox& GetCollider() const { return collider; }
};