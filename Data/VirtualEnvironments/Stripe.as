#include "VirtualEnvironments/VRSetup.as"

class Arena : ScriptObject {
    // Keep the class name ("Arena") as it is used to call the initializer

    float turningGain = 1.0f;    // Gain parameters for turning
    float walkingGain = 1.0f;    // And walking motion

    Vector3 initialPosition = Vector3(0.0f, 0.5f, 0.0f);
    Quaternion initialOrientation = Quaternion(0.0f, Vector3(0.0f, 1.0f, 0.0f));

    void Start() {
        Print("Creating stripe arena");

        /* Octree component is needed for scene functioning;
        //   without it the scene is not rendered             */
        scene.CreateComponent("Octree");

        /* Physics world is needed when you want to detect and handle collisions
        // can be commented out when collider shapes are not used */
        scene.CreateComponent("DebugRenderer");

        // The zone setting illumination/fog properties
        // You can have multiple zones if needed
        Node@ zoneNode = scene.CreateChild("Zone");
        Zone@ zone = zoneNode.CreateComponent("Zone");
        zone.boundingBox = BoundingBox(-1000.0f, 1000.0f);
        zone.fogColor = Color(1.0f,1.0f,1.0f);                                  //0.0f,0.0f,0.0f
        zone.ambientColor = Color(1.0f, 1.0f, 1.0f);
        zone.fogStart = 1000.0f;
        zone.fogEnd = 1000.0f;

        Node@ node;           //References to a node and a static model
        StaticModel@ object;  //to be recycled creating all the arena
        CollisionShape@ collider;
        RigidBody@ rigidBody;

        node = scene.CreateChild("Pole1");
        node.scale = Vector3(14.0f, 500.0f, 14.0f);
        node.position = Vector3(0.0f, 0.0f, 20.0f);
        node.Rotate(Quaternion(45.0f, Vector3(1.0f, 1.0f, 0.0f)));
        object = node.CreateComponent("StaticModel");
        object.model = cache.GetResource("Model", "Models/Cylinder.mdl");
        object.material = cache.GetResource("Material", "Materials/Black.xml");   //White.xml

        rigidBody = node.CreateComponent("RigidBody");
        //rigidBody.mass = 1.0;
        rigidBody.collisionLayer = 1;
        collider = node.CreateComponent("CollisionShape");
        collider.SetCylinder(1.1, 1);


        // Call this function common for all the arenas in the same VR setting
        // to configure screens.
        SetupViewports(
            scene,                        // reference to the scene,
            initialPosition,              // initial position and
            initialOrientation            // orientation in the VR.
        );
    }

    void PostUpdate(float timeStep)
    {
      Node@ cam = scene.GetChild("Subject");
      cam.position = Vector3(cam.position.x, 0.5, cam.position.z);
      /*
      //Print(cam.rotation.yaw);
      if (cam.rotation.yaw>146) {
        Print(cam.rotation.yaw);
        cam.Rotate(Quaternion(10.0f, 100.0f, +100.0f));
        Print(cam.rotation.yaw);
      }
      if (cam.rotation.yaw<-146) {
        cam.rotation = Quaternion(0.0f, 0.0f, 0.0f);
      }
      //RigidBody@ rb = cam.GetComponent("RigidBody");
      //rb.ApplyForce(Vector3(0.0f, 0.5f, 0.0f));
      //Print("postupdate");
        // this called every frame

        */
        physicsWorld.DrawDebugGeometry(true);
    }

}
