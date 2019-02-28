void SetupViewports(Scene scene, Vector3 initialPosition, Quaternion initialOrientation) {
    /*
    This function sets up the cameras and viewports specifically for the given
    VR screen configuration. In this case, the VR screen consists of 4
    segments, rendered onto 2 projection screens. Therefore, 4 viewports are
    created, each taking half of the screen, and 4 cameras are assigned to
    those viewports; the position of the cameras in the VR are reflecting the
    screen segments orientation relative to the experimental subject.
    _____________     _____________
    |     |     |     |     |     |
    | vp1 | vp2 |     | vp3 | vp4 |
    |_____|_____|     |_____|_____|
       Screen 1          Screen 2
    */
    renderer.numViewports = 4;


    /*
    The root node "Subject" is the one affected by motion in VR. The cameras
    are attached to it to maintain their relative orientation. Do not change
    its name, because that is how it is found by the VR application.
    */
    Node@ subjectNode = scene.CreateChild("Subject");
    subjectNode.position = initialPosition;
    subjectNode.Rotate(initialOrientation);

    /*
    The collision shape and rigid body components are needed to processing
    Collisions with other colliding objects. A sphere around the subject is used
    */
    //RigidBody@ rigidBody = subjectNode.CreateComponent("RigidBody");
    //rigidBody.mass = 1.0;
    //rigidBody.collisionLayer = 1;
    //CollisionShape@ collider = subjectNode.CreateComponent("CollisionShape");
    //collider.SetBox(Vector3(0.4f,0.4f,0.4f));
    //SubscribeToEvent(subjectNode, "NodeCollision", "HandleNodeCollision");

    Node@ cam1 = subjectNode.CreateChild("Camera_1");
    Node@ cam2 = subjectNode.CreateChild("Camera_2");
    Node@ cam3 = subjectNode.CreateChild("Camera_3");
    Node@ cam4 = subjectNode.CreateChild("Camera_4");

    cam1.Rotate(Quaternion(-108.0f, Vector3(0.0f, 1.0f, 0.0f)));
    cam2.Rotate(Quaternion(-36.0f, Vector3(0.0f, 1.0f, 0.0f)));
    cam3.Rotate(Quaternion(+36.0f, Vector3(0.0f, 1.0f, 0.0f)));
    cam4.Rotate(Quaternion(+108.0f, Vector3(0.0f, 1.0f, 0.0f)));

    Camera@ c;
    c = cam1.CreateComponent("Camera");
    c.aspectRatio = float(graphics.width/4.0f) / float(graphics.height);
    c.fov = 78.0f;

    c = cam2.CreateComponent("Camera");
    c.aspectRatio = float(graphics.width/4.0f) / float(graphics.height);
    c.fov = 78.0f;

    c = cam3.CreateComponent("Camera");
    c.aspectRatio = float(graphics.width/4.0f) / float(graphics.height);
    c.fov = 78.0f;

    c = cam4.CreateComponent("Camera");
    c.aspectRatio = float(graphics.width/4.0f) / float(graphics.height);
    c.fov = 78.0f;

    // Assigning viewports to parts of the screen
    // note that cameras starting from leftmost to rightmost
    // are not in order, this is because how the screens arena
    // physically arranged and because of the flipping by the projection
    Viewport@ vp;

    vp = Viewport(scene, cam1.GetComponent("Camera"),
      IntRect(0, 0, graphics.width*1/4, graphics.height));
    renderer.viewports[0] = vp;

    vp = Viewport(scene, cam2.GetComponent("Camera"),
      IntRect(graphics.width*1/4, 0, graphics.width*2/4, graphics.height));
    renderer.viewports[1] = vp;

    vp = Viewport(scene, cam3.GetComponent("Camera"),
      IntRect(graphics.width*2/4, 0, graphics.width*3/4, graphics.height));
    renderer.viewports[2] = vp;

    vp = Viewport(scene, cam4.GetComponent("Camera"),
      IntRect(graphics.width*3/4, 0, graphics.width, graphics.height));
    renderer.viewports[3] = vp;
}


void HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    Print("collision");
    //VectorBuffer contacts = eventData["Contacts"].GetBuffer();
    /*
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
    }
    */
}
