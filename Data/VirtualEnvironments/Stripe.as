#include "VirtualEnvironments/VRSetup.as"

class Arena : ScriptObject {
    // Keep the class name as it is used to call the initializer

    PhysicsWorld@ pw;

    void Initialize() {
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
        object = node.CreateComponent("StaticModel");
        object.model = cache.GetResource("Model", "Models/Cylinder.mdl");
        object.material = cache.GetResource("Material", "Materials/Black.xml");   //White.xml

//        node = scene.CreateChild("Pole2");
  //      node.scale = Vector3(7.0f, 500.0f, 7.0f);
    //    node.position = Vector3(0.0f, 0.0f, -20.0f);
      //  object = node.CreateComponent("StaticModel");
        //object.model = cache.GetResource("Model", "Models/Cylinder.mdl");
        //object.material = cache.GetResource("Material", "Materials/Black.xml");    //White.xml


        // Call this function common for all the arenas in the same VR setting
        // to configure screens.
        SetupViewports(
            scene,                                       // reference to the scene,
            Vector3(0.0f, 0.5f, 0.0f),                   // initial position and
            Quaternion(0.0f, Vector3(0.0f, 1.0f, 0.0f))  // orientation in the VR.
        );
    }
/*
    void PostUpdate(float timeStep)
    {
      Node@ cam = scene.GetChild("Subject");
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
        //pw.DrawDebugGeometry(true);
    }
    */
}
