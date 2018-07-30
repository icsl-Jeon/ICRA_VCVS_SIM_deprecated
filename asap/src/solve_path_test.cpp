#include "ASAP.h"


// CVXGEN
Params params;
Vars vars;
Workspace work;
Settings settings;

int main(int argc,char **argv){

    set_defaults();
    settings.eps=1e-6;
    settings.resid_tol=1e-6;
    settings.verbose=0;
    setup_indexing();



    ros::init(argc,argv,"solving_path_test_node");
    // parameter parsing
    ros::NodeHandle nh_private("~");
    asap_ns::Params asap_params;

    asap_params.azim_min=0; asap_params.azim_max=2*PI;

    float tracking_d_min,tracking_d_max;
    int N_tracking_d;
    double solving_speed;

    std::string tracker_name,target_name;

    nh_private.getParam("tracking_d_min", tracking_d_min);
    nh_private.getParam("tracking_d_max", tracking_d_max);
    nh_private.getParam("tracking_d_N",N_tracking_d);

    asap_params.set_ds(tracking_d_min,tracking_d_max,N_tracking_d);

    nh_private.getParam("elev_min", asap_params.elev_min);
    nh_private.getParam("elev_max", asap_params.elev_max);

    nh_private.getParam("N_azim", asap_params.N_azim);
    nh_private.getParam("N_elev",asap_params.N_elev);
    nh_private.getParam("local_range",asap_params.local_range);
    nh_private.getParam("N_extrema",asap_params.N_extrem);
    nh_private.getParam("N_history",asap_params.N_history);
    nh_private.getParam("N_prediction",asap_params.N_pred);
    nh_private.getParam("t_prediction",asap_params.t_pred); // horizon
    nh_private.getParam("solving_speed",solving_speed); // horizon



    nh_private.getParam("max_interval_distance",asap_params.max_interval_distance);
    nh_private.getParam("w_v0",asap_params.w_v0);
    nh_private.getParam("w_j",asap_params.w_j);
    nh_private.getParam("alpha",asap_params.alpha);
    nh_private.getParam("tracker_name",asap_params.tracker_name);
    nh_private.getParam("target_name",asap_params.target_name);


    printf("Parameters summary: \n");
    printf("------------------------------\n");
    printf("------------------------------\n");

    printf("sampling distance: [ ");
    for(int i=0;i<asap_params.tracking_ds.size();i++)
        printf("%f ",asap_params.tracking_ds[i]);
    printf(" ]\n");

    printf("azimuth range: [%f, %f] N: %d / elevation: [%f, %f] N: %d \n",asap_params.azim_min,asap_params.azim_max,
    asap_params.N_azim,asap_params.elev_min,asap_params.elev_max,asap_params.N_elev);

    printf("local maxima search in visibility matrix : search window = %d / N_extrema = %d\n",asap_params.local_range,asap_params.N_extrem);
    printf("max interval disance: %f\n",asap_params.max_interval_distance);
    printf("nominal weight for visibility in edge connection: %f \n",asap_params.w_v0);

    std::cout<<"tracker name: "<<asap_params.tracker_name<<" target_name: "<<asap_params.target_name<<std::endl;

    ASAP asap_obj(asap_params);

    ROS_INFO("Always See and Picturing started");
    ros::Rate rate(30);

    // initialize
    ros::Duration(1.0).sleep();

    // planning check point
    ros::Time planning_ckp=ros::Time::now();


    asap_obj.hovering(ros::Duration(1.0),double(1));


    // checkpoint for calculation of velocity
    ros::Time time_ckp=ros::Time::now();
    geometry_msgs::Point tracker_position_ckp=asap_obj.cur_tracker_pos;

    // main loop
    while(ros::ok()){
        if (asap_obj.octomap_callback_flag && asap_obj.state_callback_flag)
        {

            asap_obj.target_regression(); // get regression model from history
            asap_obj.target_future_prediction();

            // let's measure velocity of tracker

			ros::Time now=ros::Time::now();
	        ros::Duration dt=now-time_ckp;
           
			asap_obj.cur_tracker_vel.linear.x=(asap_obj.cur_tracker_pos.x-tracker_position_ckp.x)/dt.toSec();
            asap_obj.cur_tracker_vel.linear.y=(asap_obj.cur_tracker_pos.y-tracker_position_ckp.y)/dt.toSec();
            asap_obj.cur_tracker_vel.linear.z=(asap_obj.cur_tracker_pos.z-tracker_position_ckp.z)/dt.toSec();


            std::cout<<"velocity: [ "<<asap_obj.cur_tracker_vel.linear.x<<", "
                                     <<asap_obj.cur_tracker_vel.linear.y<<", "
                                     <<asap_obj.cur_tracker_vel.linear.z<<"]"<<std::endl;

			time_ckp=now;
			tracker_position_ckp.x=asap_obj.cur_tracker_pos.x;
			tracker_position_ckp.y=asap_obj.cur_tracker_pos.y;
			tracker_position_ckp.z=asap_obj.cur_tracker_pos.z;


            // planning once this condition is satisfied
            if(ros::Time::now().toSec()-planning_ckp.toSec()>solving_speed) {

                asap_obj.reactive_planning();
                planning_ckp=ros::Time::now();
                asap_obj.smooth_path_update();
            }

            asap_obj.quad_waypoint_pub();
            asap_obj.path_publish(); // skeleton + smooth path publish
            asap_obj.marker_publish();
            asap_obj.points_publish();
        }

        ros::spinOnce();
        rate.sleep();
    }

    return 0;

}
