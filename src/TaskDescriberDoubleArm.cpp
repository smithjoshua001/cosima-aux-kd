/* Author: Niels Dehio
 * Date:   07 December 2016
 *
 * Description: 
 */

#include "TaskDescriberDoubleArm.hpp"
#include <rtt/Component.hpp> // needed for the macro at the end of this file


TaskDescriberDoubleArm::TaskDescriberDoubleArm(std::string const & name) : RTT::TaskContext(name) {
    //prepare operations
    addOperation("setDOFsize", &TaskDescriberDoubleArm::setDOFsize, this).doc("set DOF size");
    addOperation("loadModel", &TaskDescriberDoubleArm::loadModel, this).doc("load model");
    addOperation("displayCurrentState", &TaskDescriberDoubleArm::displayCurrentState, this).doc("print current state");

    //other stuff
    portsArePrepared = false;
}

bool TaskDescriberDoubleArm::configureHook() {
    // intializations and object creations go here. Each component should run this before being able to run

    //check conncetion of input port, the output ports must not necessarily be connected
//    if (!in_robotstatus_port.connected())
//        return false;
//    else
        return true;
}

bool TaskDescriberDoubleArm::startHook() {
    // this method starts the component
    return true;
}

void TaskDescriberDoubleArm::updateHook() {
    // this is the actual body of a component. it is called on each cycle
    if (in_robotstatus_port.connected()) {
        in_robotstatus_flow = in_robotstatus_port.read(in_robotstatus_var);
    } else {
        // handle the situation
    }

    if (in_jacobian_port.connected()) {
        in_jacobian_flow = in_jacobian_port.read(in_jacobian_var);
    } else {
        // handle the situation
    }

    if (in_jacobianDot_port.connected()) {
        in_jacobianDot_flow = in_jacobianDot_port.read(in_jacobianDot_var);
    } else {
        // handle the situation
    }

    // you can handle cases when there is no new data.
    if ( (in_robotstatus_flow == RTT::NewData || in_robotstatus_flow == RTT::OldData) &&
         (in_jacobian_flow == RTT::NewData || in_jacobian_flow == RTT::OldData) &&
         (in_jacobianDot_flow == RTT::NewData || in_jacobianDot_flow == RTT::OldData) ){

        assert(in_robotstatus_var.angles.rows()==DOFsize);
        assert(in_robotstatus_var.angles.cols()==1);
        assert(in_robotstatus_var.velocities.rows()==DOFsize);
        assert(in_robotstatus_var.velocities.cols()==1);
        assert(in_robotstatus_var.torques.rows()==DOFsize);
        assert(in_robotstatus_var.torques.cols()==1);

        assert(in_jacobian_var.rows()==12);
        assert(in_jacobian_var.cols()==DOFsize);

        assert(in_jacobianDot_var.rows()==12);
        assert(in_jacobianDot_var.cols()==DOFsize);

        // specify jacobian full
        out_jacobianFull_var = in_jacobian_var;
        out_jacobianDotFull_var = in_jacobianDot_var;

        
        // specify jacobian task
        out_jacobianTask_var = in_jacobian_var;
        out_jacobianDotTask_var = in_jacobianDot_var;


        // specify jacobian constrained
        jntPosConfigPlusJntVelConfig_q.q.data.setZero();
        jntPosConfigPlusJntVelConfig_q.qdot.data.setZero();
        this->castEigenVectorFtoD(in_robotstatus_var.angles, jntPosConfigPlusJntVelConfig_q.q.data);
        this->castEigenVectorFtoD(in_robotstatus_var.velocities, jntPosConfigPlusJntVelConfig_q.qdot.data);

        jntPosConfigPlusJntVelConfig_q.q.data(0) = in_robotstatus_var.angles(6);//reverse order of left arm joint values
        jntPosConfigPlusJntVelConfig_q.q.data(1) = in_robotstatus_var.angles(5);
        jntPosConfigPlusJntVelConfig_q.q.data(2) = in_robotstatus_var.angles(4);
        jntPosConfigPlusJntVelConfig_q.q.data(3) = in_robotstatus_var.angles(3);
        jntPosConfigPlusJntVelConfig_q.q.data(4) = in_robotstatus_var.angles(2);
        jntPosConfigPlusJntVelConfig_q.q.data(5) = in_robotstatus_var.angles(1);
        jntPosConfigPlusJntVelConfig_q.q.data(6) = in_robotstatus_var.angles(0);

        jntPosConfigPlusJntVelConfig_q.qdot.data(0) = in_robotstatus_var.velocities(6);//reverse order of left arm joint values
        jntPosConfigPlusJntVelConfig_q.qdot.data(1) = in_robotstatus_var.velocities(5);
        jntPosConfigPlusJntVelConfig_q.qdot.data(2) = in_robotstatus_var.velocities(4);
        jntPosConfigPlusJntVelConfig_q.qdot.data(3) = in_robotstatus_var.velocities(3);
        jntPosConfigPlusJntVelConfig_q.qdot.data(4) = in_robotstatus_var.velocities(2);
        jntPosConfigPlusJntVelConfig_q.qdot.data(5) = in_robotstatus_var.velocities(1);
        jntPosConfigPlusJntVelConfig_q.qdot.data(6) = in_robotstatus_var.velocities(0);

        jac_.data.setZero();
        jac_dot_.data.setZero();
        jnt_to_jac_solver->JntToJac(jntPosConfigPlusJntVelConfig_q.q, jac_, kdl_chain_.getNrOfSegments());
        jnt_to_jac_dot_solver->JntToJacDot(jntPosConfigPlusJntVelConfig_q, jac_dot_, kdl_chain_.getNrOfSegments());

        this->castEigenMatrixDtoF(jac_.data, out_jacobianCstr_var);
        this->castEigenMatrixDtoF(jac_dot_.data, out_jacobianDotCstr_var);


        // seperate in Translation and Orientation
        out_jacobianTranslationFull_var = out_jacobianFull_var.topRows<6>();
        out_jacobianDotTranslationFull_var = out_jacobianDotFull_var.topRows<6>();
        out_jacobianOrientationFull_var = out_jacobianFull_var.bottomRows<6>();
        out_jacobianDotOrientationFull_var = out_jacobianDotFull_var.bottomRows<6>();

        out_jacobianTranslationTask_var = out_jacobianTask_var.topRows<6>();
        out_jacobianDotTranslationTask_var = out_jacobianDotTask_var.topRows<6>();
        out_jacobianOrientationTask_var = out_jacobianTask_var.bottomRows<6>();
        out_jacobianDotOrientationTask_var = out_jacobianDotTask_var.bottomRows<6>();

        out_jacobianTranslationCstr_var = out_jacobianCstr_var.topRows<3>();
        out_jacobianDotTranslationCstr_var = out_jacobianDotCstr_var.topRows<3>();
        out_jacobianOrientationCstr_var = out_jacobianCstr_var.bottomRows<3>();
        out_jacobianDotOrientationCstr_var = out_jacobianDotCstr_var.bottomRows<3>();

    } else if ( (in_robotstatus_flow == RTT::NoData) || (in_jacobian_flow == RTT::NoData) || (in_jacobianDot_flow == RTT::NoData) ) {
        out_jacobianTask_var.setZero();
        out_jacobianDotTask_var.setZero();
        out_jacobianTranslationTask_var.setZero();
        out_jacobianDotTranslationTask_var.setZero();
        out_jacobianOrientationTask_var.setZero();
        out_jacobianDotOrientationTask_var.setZero();

        out_jacobianCstr_var.setZero();
        out_jacobianDotCstr_var.setZero();
        out_jacobianTranslationCstr_var.setZero();
        out_jacobianDotTranslationCstr_var.setZero();
        out_jacobianOrientationCstr_var.setZero();
        out_jacobianDotOrientationCstr_var.setZero();
    } else {
        // there should be something really wrong!
    }

    out_jacobianFull_port.write(out_jacobianFull_var);
    out_jacobianDotFull_port.write(out_jacobianDotFull_var);
    out_jacobianTranslationFull_port.write(out_jacobianTranslationFull_var);
    out_jacobianDotTranslationFull_port.write(out_jacobianDotTranslationFull_var);
    out_jacobianOrientationFull_port.write(out_jacobianOrientationFull_var);
    out_jacobianDotOrientationFull_port.write(out_jacobianDotOrientationFull_var);

    out_jacobianTask_port.write(out_jacobianTask_var);
    out_jacobianDotTask_port.write(out_jacobianDotTask_var);
    out_jacobianTranslationTask_port.write(out_jacobianTranslationTask_var);
    out_jacobianDotTranslationTask_port.write(out_jacobianDotTranslationTask_var);
    out_jacobianOrientationTask_port.write(out_jacobianOrientationTask_var);
    out_jacobianDotOrientationTask_port.write(out_jacobianDotOrientationTask_var);

    out_jacobianCstr_port.write(out_jacobianCstr_var);
    out_jacobianDotCstr_port.write(out_jacobianDotCstr_var);
    out_jacobianTranslationCstr_port.write(out_jacobianTranslationCstr_var);
    out_jacobianDotTranslationCstr_port.write(out_jacobianDotTranslationCstr_var);
    out_jacobianOrientationCstr_port.write(out_jacobianOrientationCstr_var);
    out_jacobianDotOrientationCstr_port.write(out_jacobianDotOrientationCstr_var);
}

void TaskDescriberDoubleArm::stopHook() {
    // stops the component (update hook wont be  called anymore)
}

void TaskDescriberDoubleArm::cleanupHook() {
    // cleaning the component data
    portsArePrepared = false;
}

void TaskDescriberDoubleArm::setDOFsize(unsigned int DOFsize){
    assert(DOFsize > 0);
    this->DOFsize = DOFsize;

    jntPosConfigPlusJntVelConfig_q = KDL::JntArrayVel(kdl_chain_.getNrOfJoints());
    jac_ = KDL::Jacobian(kdl_chain_.getNrOfJoints());
    jac_dot_ = KDL::Jacobian(kdl_chain_.getNrOfJoints());

    this->preparePorts();
}


void TaskDescriberDoubleArm::loadModel(std::string modelname, std::string chain_root_link_name, std::string chain_tip_link_name){
    assert(modelname.length() > 0);
    assert(chain_root_link_name.length() > 0);
    assert(chain_tip_link_name.length() > 0);
    assert(exists_test(modelname) == true);

    //init KDL tree and KDL chain
    //    robot_model = urdf::Model();
    if (!robot_model.initFile(modelname.c_str())) {
        std::cerr << "Could not generate robot model" << std::endl;
    }
    //    kdl_tree = KDL::Tree();
    if (!kdl_parser::treeFromUrdfModel(robot_model, kdl_tree)) {
        std::cerr << "Could not extract kdl tree" << std::endl;
    }
    assert(kdl_tree.getNrOfJoints() > 0);
    //    kdl_chain_ = KDL::Chain();
    kdl_tree.getChain(chain_root_link_name, chain_tip_link_name, kdl_chain_);
    assert(kdl_chain_.getNrOfJoints() > 0);

    // initialize KDL solver
    jnt_to_jac_solver.reset(new KDL::ChainJntToJacSolver(kdl_chain_));
    jnt_to_jac_dot_solver.reset(new KDL::ChainJntToJacDotSolver(kdl_chain_));

    std::cout << " tree NrOfJoints " << kdl_tree.getNrOfJoints() << std::endl;
    std::cout << " tree getNrOfSegments " << kdl_tree.getNrOfSegments() << std::endl;
    std::cout << " chain NrOfJoints " << kdl_chain_.getNrOfJoints() << std::endl;
    std::cout << " chain getNrOfSegments " << kdl_chain_.getNrOfSegments() << std::endl;
}


void TaskDescriberDoubleArm::preparePorts(){
    if (portsArePrepared){
        ports()->removePort("in_robotstatus_port");
        ports()->removePort("in_jacobian_port");
        ports()->removePort("in_jacobianDot_port");

        ports()->removePort("out_jacobianFull_port");
        ports()->removePort("out_jacobianDotFull_port");
        ports()->removePort("out_jacobianTranslationFull_port");
        ports()->removePort("out_jacobianDotTranslationFull_port");
        ports()->removePort("out_jacobianOrientationFull_port");
        ports()->removePort("out_jacobianDotOrientationFull_port");

        ports()->removePort("out_jacobianTask_port");
        ports()->removePort("out_jacobianDotTask_port");
        ports()->removePort("out_jacobianTranslationTask_port");
        ports()->removePort("out_jacobianDotTranslationTask_port");
        ports()->removePort("out_jacobianOrientationTask_port");
        ports()->removePort("out_jacobianDotOrientationTask_port");

        ports()->removePort("out_jacobianCstr_port");
        ports()->removePort("out_jacobianDotCstr_port");
        ports()->removePort("out_jacobianTranslationCstr_port");
        ports()->removePort("out_jacobianDotTranslationCstr_port");
        ports()->removePort("out_jacobianOrientationCstr_port");
        ports()->removePort("out_jacobianDotOrientationCstr_port");
    }

    //prepare input
    in_robotstatus_var = rstrt::robot::JointState(DOFsize);
    in_robotstatus_port.setName("in_robotstatus_port");
    in_robotstatus_port.doc("Input port for robotstatus values");
    ports()->addPort(in_robotstatus_port);
    in_robotstatus_flow = RTT::NoData;

    in_jacobian_var = Eigen::MatrixXf(12,DOFsize);
    in_jacobian_var.setZero();
    in_jacobian_port.setName("in_jacobian_port");
    in_jacobian_port.doc("Input port for jacobian matrix");
    ports()->addPort(in_jacobian_port);
    in_jacobian_flow = RTT::NoData;

    in_jacobianDot_var = Eigen::MatrixXf(12,DOFsize);
    in_jacobianDot_var.setZero();
    in_jacobianDot_port.setName("in_jacobianDot_port");
    in_jacobianDot_port.doc("Input port for jacobianDot matrix");
    ports()->addPort(in_jacobianDot_port);
    in_jacobianDot_flow = RTT::NoData;

    //prepare output
    out_jacobianFull_var = Eigen::MatrixXf(12,DOFsize);
    out_jacobianFull_var.setZero();
    out_jacobianFull_port.setName("out_jacobianFull_port");
    out_jacobianFull_port.doc("Output port for jacobian matrix");
    out_jacobianFull_port.setDataSample(out_jacobianFull_var);
    ports()->addPort(out_jacobianFull_port);

    out_jacobianDotFull_var = Eigen::MatrixXf(12,DOFsize);
    out_jacobianDotFull_var.setZero();
    out_jacobianDotFull_port.setName("out_jacobianDotFull_port");
    out_jacobianDotFull_port.doc("Output port for jacobianDot matrix");
    out_jacobianDotFull_port.setDataSample(out_jacobianDotFull_var);
    ports()->addPort(out_jacobianDotFull_port);

    out_jacobianTranslationFull_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianTranslationFull_var.setZero();
    out_jacobianTranslationFull_port.setName("out_jacobianTranslationFull_port");
    out_jacobianTranslationFull_port.doc("Output port for jacobianTranslation matrix");
    out_jacobianTranslationFull_port.setDataSample(out_jacobianTranslationFull_var);
    ports()->addPort(out_jacobianTranslationFull_port);

    out_jacobianDotTranslationFull_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianDotTranslationFull_var.setZero();
    out_jacobianDotTranslationFull_port.setName("out_jacobianDotTranslationFull_port");
    out_jacobianDotTranslationFull_port.doc("Output port for jacobianDotTranslation matrix");
    out_jacobianDotTranslationFull_port.setDataSample(out_jacobianDotTranslationFull_var);
    ports()->addPort(out_jacobianDotTranslationFull_port);

    out_jacobianOrientationFull_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianOrientationFull_var.setZero();
    out_jacobianOrientationFull_port.setName("out_jacobianOrientationFull_port");
    out_jacobianOrientationFull_port.doc("Output port for jacobianOrientation matrix");
    out_jacobianOrientationFull_port.setDataSample(out_jacobianOrientationFull_var);
    ports()->addPort(out_jacobianOrientationFull_port);

    out_jacobianDotOrientationFull_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianDotOrientationFull_var.setZero();
    out_jacobianDotOrientationFull_port.setName("out_jacobianDotOrientationFull_port");
    out_jacobianDotOrientationFull_port.doc("Output port for jacobianDotOrientation matrix");
    out_jacobianDotOrientationFull_port.setDataSample(out_jacobianDotOrientationFull_var);
    ports()->addPort(out_jacobianDotOrientationFull_port);

    // ###

    out_jacobianTask_var = Eigen::MatrixXf(12,DOFsize);
    out_jacobianTask_var.setZero();
    out_jacobianTask_port.setName("out_jacobianTask_port");
    out_jacobianTask_port.doc("Output port for jacobian matrix");
    out_jacobianTask_port.setDataSample(out_jacobianTask_var);
    ports()->addPort(out_jacobianTask_port);

    out_jacobianDotTask_var = Eigen::MatrixXf(12,DOFsize);
    out_jacobianDotTask_var.setZero();
    out_jacobianDotTask_port.setName("out_jacobianDotTask_port");
    out_jacobianDotTask_port.doc("Output port for jacobianDot matrix");
    out_jacobianDotTask_port.setDataSample(out_jacobianDotTask_var);
    ports()->addPort(out_jacobianDotTask_port);

    out_jacobianTranslationTask_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianTranslationTask_var.setZero();
    out_jacobianTranslationTask_port.setName("out_jacobianTranslationTask_port");
    out_jacobianTranslationTask_port.doc("Output port for jacobianTranslation matrix");
    out_jacobianTranslationTask_port.setDataSample(out_jacobianTranslationTask_var);
    ports()->addPort(out_jacobianTranslationTask_port);

    out_jacobianDotTranslationTask_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianDotTranslationTask_var.setZero();
    out_jacobianDotTranslationTask_port.setName("out_jacobianDotTranslationTask_port");
    out_jacobianDotTranslationTask_port.doc("Output port for jacobianDotTranslation matrix");
    out_jacobianDotTranslationTask_port.setDataSample(out_jacobianDotTranslationTask_var);
    ports()->addPort(out_jacobianDotTranslationTask_port);

    out_jacobianOrientationTask_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianOrientationTask_var.setZero();
    out_jacobianOrientationTask_port.setName("out_jacobianOrientationTask_port");
    out_jacobianOrientationTask_port.doc("Output port for jacobianOrientation matrix");
    out_jacobianOrientationTask_port.setDataSample(out_jacobianOrientationTask_var);
    ports()->addPort(out_jacobianOrientationTask_port);

    out_jacobianDotOrientationTask_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianDotOrientationTask_var.setZero();
    out_jacobianDotOrientationTask_port.setName("out_jacobianDotOrientationTask_port");
    out_jacobianDotOrientationTask_port.doc("Output port for jacobianDotOrientation matrix");
    out_jacobianDotOrientationTask_port.setDataSample(out_jacobianDotOrientationTask_var);
    ports()->addPort(out_jacobianDotOrientationTask_port);

    //####

    out_jacobianCstr_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianCstr_var.setZero();
    out_jacobianCstr_port.setName("out_jacobianCstr_port");
    out_jacobianCstr_port.doc("Output port for jacobian matrix");
    out_jacobianCstr_port.setDataSample(out_jacobianCstr_var);
    ports()->addPort(out_jacobianCstr_port);

    out_jacobianDotCstr_var = Eigen::MatrixXf(6,DOFsize);
    out_jacobianDotCstr_var.setZero();
    out_jacobianDotCstr_port.setName("out_jacobianDotCstr_port");
    out_jacobianDotCstr_port.doc("Output port for jacobianDot matrix");
    out_jacobianDotCstr_port.setDataSample(out_jacobianDotCstr_var);
    ports()->addPort(out_jacobianDotCstr_port);

    out_jacobianTranslationCstr_var = Eigen::MatrixXf(3,DOFsize);
    out_jacobianTranslationCstr_var.setZero();
    out_jacobianTranslationCstr_port.setName("out_jacobianTranslationCstr_port");
    out_jacobianTranslationCstr_port.doc("Output port for jacobianTranslation matrix");
    out_jacobianTranslationCstr_port.setDataSample(out_jacobianTranslationCstr_var);
    ports()->addPort(out_jacobianTranslationCstr_port);

    out_jacobianDotTranslationCstr_var = Eigen::MatrixXf(3,DOFsize);
    out_jacobianDotTranslationCstr_var.setZero();
    out_jacobianDotTranslationCstr_port.setName("out_jacobianDotTranslationCstr_port");
    out_jacobianDotTranslationCstr_port.doc("Output port for jacobianDotTranslation matrix");
    out_jacobianDotTranslationCstr_port.setDataSample(out_jacobianDotTranslationCstr_var);
    ports()->addPort(out_jacobianDotTranslationCstr_port);

    out_jacobianOrientationCstr_var = Eigen::MatrixXf(3,DOFsize);
    out_jacobianOrientationCstr_var.setZero();
    out_jacobianOrientationCstr_port.setName("out_jacobianOrientationCstr_port");
    out_jacobianOrientationCstr_port.doc("Output port for jacobianOrientation matrix");
    out_jacobianOrientationCstr_port.setDataSample(out_jacobianOrientationCstr_var);
    ports()->addPort(out_jacobianOrientationCstr_port);

    out_jacobianDotOrientationCstr_var = Eigen::MatrixXf(3,DOFsize);
    out_jacobianDotOrientationCstr_var.setZero();
    out_jacobianDotOrientationCstr_port.setName("out_jacobianDotOrientationCstr_port");
    out_jacobianDotOrientationCstr_port.doc("Output port for jacobianDotOrientation matrix");
    out_jacobianDotOrientationCstr_port.setDataSample(out_jacobianDotOrientationCstr_var);
    ports()->addPort(out_jacobianDotOrientationCstr_port);

    portsArePrepared = true;
}

bool TaskDescriberDoubleArm::exists_test(const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

void TaskDescriberDoubleArm::castEigenVectorDtoF(Eigen::VectorXd const & d, Eigen::VectorXf & f) {
    f = d.cast <float> ();
}

void TaskDescriberDoubleArm::castEigenVectorFtoD(Eigen::VectorXf const & f, Eigen::VectorXd & d) {
    d = f.cast <double> ();
}

void TaskDescriberDoubleArm::castEigenMatrixDtoF(Eigen::MatrixXd const & d, Eigen::MatrixXf & f) {
    f = d.cast <float> ();
}

void TaskDescriberDoubleArm::castEigenMatrixFtoD(Eigen::MatrixXf const & f, Eigen::MatrixXd & d) {
    d = f.cast <double> ();
}

void TaskDescriberDoubleArm::displayCurrentState() {
    std::cout << "############## TaskDescriberDoubleArm State begin " << std::endl;
    std::cout << " angles " << in_robotstatus_var.angles << std::endl;
    std::cout << " velocities " << in_robotstatus_var.velocities << std::endl;

    std::cout << " jacobian full " << out_jacobianFull_var << std::endl;
    std::cout << " jacobianDot full " << out_jacobianDotFull_var << std::endl;

    std::cout << " jacobian task " << out_jacobianTask_var << std::endl;
    std::cout << " jacobianDot task " << out_jacobianDotTask_var << std::endl;

    std::cout << " jacobian cstr " << out_jacobianCstr_var << std::endl;
    std::cout << " jacobianDot cstr " << out_jacobianDotCstr_var << std::endl;
    std::cout << "############## TaskDescriberDoubleArm State end " << std::endl;
}


//this macro should appear only once per library
//ORO_CREATE_COMPONENT_LIBRARY()

// This macro, as you can see, creates the component. Every component should have this!
ORO_LIST_COMPONENT_TYPE(TaskDescriberDoubleArm)
