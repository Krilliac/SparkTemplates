// SpinnerController.as — Example AngelScript for SparkEngine
// This script is loaded at runtime and can be hot-reloaded with F5.

class SpinnerController
{
    float rotSpeed = 120.0f;  // degrees per second
    float elapsed = 0.0f;

    void Start()
    {
        ASPrint("SpinnerController initialized!");
    }

    void Update(float dt)
    {
        elapsed += dt;

        Transform@ t = ASGetTransform();
        if (t !is null)
        {
            t.rotation.y += rotSpeed * dt;
            t.rotation.x = sin(elapsed * 2.0f) * 15.0f;
        }

        // Example: check for input
        if (ASGetKeyDown(0x20)) // VK_SPACE
        {
            rotSpeed = -rotSpeed; // reverse on space
            ASPrint("Rotation reversed!");
        }
    }

    void OnCollision(uint other)
    {
        ASPrint("SpinnerController collided with entity " + other);
    }
}
