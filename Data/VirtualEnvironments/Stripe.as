#include "VirtualEnvironments/VRSetup.as"

class Arena : ScriptObject {
    // Keep the class name ("Arena") as it is used to call the initializer

    float turningGain = 1.0f;    // Gain parameters for turning
    float walkingGain = 1.0f;    // And walking motion

    Vector3 initialPosition = Vector3(0.0f, 0.5f, 0.0f);  // initial position
    Quaternion initialOrientation = Quaternion(           // and orientation
        0.0f, Vector3(0.0f, 1.0f, 0.0f)                   // in the virtual
    );                                                    // environment.

    Node@ _subjectNode;    // Keeping a reference to the subject to access
                          // its location etc...

    void Start() {
        /*  This function is called on the startup of the arena.
            All arena objects are to be created here, events subscribed,
            etc.
        */
        Print("Creating stripe arena");

        scene.CreateComponent("DebugRenderer");

        // The zone setting illumination/fog properties
        // You can have multiple zones if needed
        Node@ zoneNode = scene.CreateChild("Zone");
        Zone@ zone = zoneNode.CreateComponent("Zone");
        zone.boundingBox = BoundingBox(-1000.0f, 1000.0f);
        zone.fogColor = Color(1.0f,1.0f,1.0f);             //0.0f,0.0f,0.0f
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

        rigidBody = node.CreateComponent("RigidBody");      // Rigid body and
        rigidBody.collisionLayer = 1;                       // Collision shape
        collider = node.CreateComponent("CollisionShape");  // are  both needed
        collider.SetCylinder(1.1, 1);                       // for physics simulation
                                                            // by Urho3D


        // Call this function common for all the arenas in the same VR setting
        // to configure screens.
        _subjectNode = SetupViewports(
            scene,                        // reference to the scene,
            initialPosition,              // initial position and
            initialOrientation            // orientation in the VR.
        );

        // Subscribe to collision events of the subject node.
        SubscribeToEvent(_subjectNode, "NodeCollision", "HandleNodeCollision");
    }


    void HandleNodeCollision(StringHash eventType, VariantMap& eventData)
    {
        Print("collision");
        /*
        VectorBuffer contacts = eventData["Contacts"].GetBuffer();
        while (!contacts.eof)
        {
            Vector3 contactPosition = contacts.ReadVector3();
            Vector3 contactNormal = contacts.ReadVector3();
            float contactDistance = contacts.ReadFloat();
            float contactImpulse = contacts.ReadFloat();

            // If contact is below node center and mostly vertical, assume it's a ground contact
            if (contactPosition.y < (node.position.y + 1.0f))
            {
                float level = Abs(contactNormal.y);
                if (level > 0.75)
                    onGround = true;
            }
        }*/
    }


    void PostUpdate(float timeStep)
    {
        //Conditions, etc...
        physicsWorld.DrawDebugGeometry(true);
    }
}
