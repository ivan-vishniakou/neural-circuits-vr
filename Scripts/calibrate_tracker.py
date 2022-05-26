from __future__ import print_function
import cv2
import numpy as np
import os
from tkinter import filedialog, messagebox


def lin_regress(x, y):
    A = np.vstack([x]).T
    return np.linalg.lstsq(A, y)[0]

def get_track(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
    print(lines[0])
    lines = np.array([
        [*map(float, _.split(' ')[-5:])]
        for _ in lines[1:]])
    return lines


def click_and_drag(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        #refPt = [(x, y)]
        param[3] = x;
        param[4] = y;
    elif event == cv2.EVENT_MOUSEMOVE and flags>0:
        param[0] = int(round((x+param[3])/2.0))
        param[1] = int(round((y+param[4])/2.0))
        param[2] = np.sqrt((param[3]-x)**2+(param[4]-y)**2)/2


def get_tracker_parameters(footage_file):
    print("Click and drag on the image to select where the tracked ball is located and then press Enter")
    try:
        center_x_y_rad = np.zeros(5)

        cv2.namedWindow("footage")
        cv2.setMouseCallback("footage", click_and_drag, param = center_x_y_rad)

        cap = cv2.VideoCapture(footage_file)
        k = 0
        while (cap.isOpened()):
            ret, frame = cap.read()
            if ret == True:
                frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                cv2.circle(frame,(
                    int(round(center_x_y_rad[0])),
                    int(round(center_x_y_rad[1]))),
                    int(round(center_x_y_rad[2])
                        ), color=(255))
                cv2.imshow('footage', frame)
                k = cv2.waitKey(25) & 0xFF
                if k == ord('q'):
                    break
                elif k==13: #enter pressed, success
                    cap.release()
                    center_x_y_rad[3:5] = frame.shape
                    cv2.destroyWindow('footage')
                    return center_x_y_rad
            else:
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
    except Exception as e:
        print("Error in footage processing of {}".format(footage_file))
        print(e.message)
    pass


def save_tracking_params(ball_cx, ball_cy, ball_r, cxy_rad, cxy_tan, c_z):
    lines = []
    config_filename = '..\\Config\\tracking.cfg'
    if os.path.exists(config_filename):
        with open(config_filename, 'r') as f:
            lines = f.readlines()
    else:
        #default values
        lines = """[camera settings]
# if true, CameraSettingsFile will be loaded:
LoadCameraSettings=true

#	file of the camera settings. The camera can be set up with the
#	Pylon viewer:
CameraSettingsFile=Config/camera_settings.pfs

# 	user defined name of the camera to use for ball tracking:
TrackingCameraName=ball_cam

[tracking settings]
# 	These are the settings to extract optical flow from the frame
# 	the circle bounding the tracked ball in frame coordinates:
BallCenterX=112.0
BallCenterY=70.0
BallRadius=116.0

# 	These are the calibration constants, the factors scaling optical
# 	flow in pixels to the ball rotation. [px/rad]
CxyRad=100.31
CxyTan=76.85
Cz=20.63
        """.split('\n')
        lines = [*map(lambda l: l+'\n', lines)]
    #print(lines)
    for _ in range(len(lines)):
        if "BallCenterX" in lines[_]:
            lines[_] = "BallCenterX={}\n".format(ball_cx)
        if "BallCenterY" in lines[_]:
            lines[_] = "BallCenterY={}\n".format(ball_cy)
        if "BallRadius" in lines[_]:
            lines[_] = "BallRadius={}\n".format(ball_r)

        if "CxyRad" in lines[_]:
            lines[_] = "CxyRad={}\n".format(cxy_rad)
        if "CxyTan" in lines[_]:
            lines[_] = "CxyTan={}\n".format(cxy_tan)
        if "Cz" in lines[_]:
            lines[_] = "Cz={}\n".format(c_z)

    if os.path.exists(config_filename):
        os.remove(config_filename)

    with open(config_filename, "w+") as f:
        f.writelines(lines)


def run_tracking(input_filename, parameters):
    tmp_cfg = 'tmp_track.cfg'
    tmp_track = '.'.join(input_filename.split('.')[:-1])+'.txt'
    for f in [tmp_cfg, tmp_track]:
        if os.path.exists(f):
            os.remove(f)
    with open(tmp_cfg, "w+") as f:
        f.write("""[tracking settings]
BallCenterX={}
BallCenterY={}
BallRadius={}
CxyRad=1.0
CxyTan=1.0
Cz=1.0
""".format(*parameters[:3]))
    os.system(r'..\BallFootageTracking.exe "{}" "{}" -c "{}" -q true'.format(input_filename, tmp_track, tmp_cfg))
    if os.path.exists(tmp_cfg):
        os.remove(tmp_cfg)
    return tmp_track

def main():
    print("Welcome to the 2-camera calibration procedure")
    print("Select main (ball tracking) camera footage")
    messagebox.showinfo("Select calibration footage", "Select the footage from the main tracking camera")
    main_footage_file = filedialog.askopenfilename()
    main_track_params = get_tracker_parameters(main_footage_file)
    print('main: ', main_track_params)
    if main_track_params is not None:
        main_track_file = run_tracking(main_footage_file, main_track_params)
    else:
        print("Could not get tracking parameters")
        return
    print("Select secondary camera footage")
    messagebox.showinfo("Select calibration footage", "Select the footage from the secondary camera")
    secondary_footage_file = filedialog.askopenfilename(initialdir=main_footage_file)
    secondary_track_params = get_tracker_parameters(secondary_footage_file)
    print('secondary: ', secondary_track_params)
    if secondary_track_params is not None:
        secondary_track_file = run_tracking(secondary_footage_file, secondary_track_params)
    else:
        print("Could not get tracking parameters")
        return

    '''
    Caluclating the calibration factors
    '''
    main_track = get_track(main_track_file)
    secondary_track = get_track(secondary_track_file)
    print("number of lines of main track = ", len(main_track))
    print("number of lines of secondary track = ", len(secondary_track))

    min_lines = min(len(main_track), len(secondary_track))
    print("number of lines to be used = ", min_lines)
    main_track = main_track[0:min_lines, :]
    secondary_track = secondary_track[0:min_lines, :]

    fit_quality_threshold = np.mean(main_track[:, 0])
    filter = np.squeeze(np.where((np.abs(main_track[:, 0]) < fit_quality_threshold) & (np.abs(secondary_track[:, 0]) < fit_quality_threshold)))
    
    main_track, secondary_track = main_track[filter, :], secondary_track[filter, :]

    main_tracking_cam_resolution_h = main_track_params[3]
    secondary_tracking_cam_resolution_h = secondary_track_params[3]

    c_z = float(main_tracking_cam_resolution_h) / np.pi / 2.0
    c_z_secondary = float(secondary_tracking_cam_resolution_h)/np.pi/2.0

    rad_tan_ratio = lin_regress(main_track[:, 1], main_track[:, 2])[0]
    c = lin_regress(np.cos(main_track[:, 4]) * main_track[:, 2], secondary_track[:, 3])[0]

    c_xy_rad = np.abs(c_z_secondary/c)
    c_xy_tan = np.abs(c_xy_rad * rad_tan_ratio)

    #print(rad_tan_ratio)

    print(c_xy_rad, c_xy_tan, c_z)

    mbox = messagebox.askquestion('Calibration complete. Save parameters?',
            'Calibration complete. The factors found are: \nc_xy_rad={}\nc_xy_tan={}\n c_z={}\nSave them for VR application?'.format(
                c_xy_rad, c_xy_tan, c_z
            ),
            icon='info')
    if mbox == 'yes':
        save_tracking_params(main_track_params[0],
                             main_track_params[1],
                             main_track_params[2],
                             c_xy_rad,
                             c_xy_tan,
                             c_z
                             )
        pass
    else:
        print("No :(")


    #secondary_footage_file = filedialog.askopenfilename(initialdir=main_footage_file)
    #main_footage_file = 'D:\\test.avi'
    print(main_footage_file)






    pass

if __name__=="__main__":
    main()
    print("Press any key to exit")
    #while cv2.waitKey(1) == 255:
    #    pass
