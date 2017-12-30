#include <iostream>
#include <vector>
#include <algorithm>
#include "coast_line.h"
#include "coast_node.h"
#include "point.h"
#include "Header.h"
#include "site.h"
#include "Eventqueue.h"

using namespace std;

bool in_face(point p, face* f) {
	half_edge* e = f->inc_edge;
	do
	{
		point p1(e->ori->x, e->ori->y);
		point p2(e->twin->ori->x, e->twin->ori->y);
		e = e->pre;
		if (!to_left(p2, p1, p)) { return false; };
	} while (e && e!=f->inc_edge);
	e = f->inc_edge;
	do {
		point p1(e->ori->x, e->ori->y);
		point p2(e->twin->ori->x, e->twin->ori->y);
		e = e->succ;
		if (!to_left(p2, p1, p)) { return false; };
	} while (e && e != f->inc_edge);
	return true;
}

bool add_circle_event(coast_node* work, eventqueue& circle_event_queue, double position) {
	cir_res r1 = circle_detect(work);
	if (r1.success && r1.circle_point.y <= position) {
		Event_circle* s1 = new Event_circle(r1.circle_point.x, r1.circle_point.y);
		s1->node = work;
		s1->center = r1.circle_center;
		s1->circle[0] = work->pred->leaf.id;
		s1->circle[1] = work->leaf.id;
		s1->circle[2] = work->suc->leaf.id;
		circle_event_queue.insert(s1);
		work->re_event = s1;
		return true;
	};
	return false;
}

bool in_box(double x, double y) {
	return vx_min <= x && x <= vx_max && vy_min <= y && y <= vy_max;
}

void traverse(coast_node* work, double po, vector<vertex* >& vertex_vec) {
	if (work->is_leaf()) return;
	traverse(work->lc, po, vertex_vec);
	double x, y, x0, y0, dx, dy;
	do
	{
		x = work->break_point(po);
		x0 = work->p1.p.x;
		y0 = work->p1.p.y;
		y = ((x - x0)*(x - x0) + y0*y0 - po*po) / (2 * (y0-po));
		dx = x - x0;
		dy = y - y0;
		po -= 100;
	} while (in_box(x, y));

	vertex* v = new vertex(vertex_vec.size(), x, y);
	work->edge->ori = v;
	v->inc_edge = work->edge;
	vertex_vec.push_back(v);
	traverse(work->rc, po, vertex_vec);
}

void read(long int & number, vector<site>& site_point, long int & test_point_num, vector<point>& test_point) {
	cin >> number;
	long int s = 0;
	double a, b;
	while (s < number) {
		cin >> a >> b;
		a = a / 10;
		b = b / 10;
		site to_add(a, b, s++);
		site_point.push_back(to_add);
	}
	cin >> test_point_num;
	s = 0;
	while (s<test_point_num) {
		cin >> a >> b;
		a = a / 10;
		b = b / 10;
		s++;
		point to_add(a, b);
		test_point.push_back(to_add);
	}
}

int main() {
	long int number;
	long int test_point_number;
	vector<point> test_point;
	vector<half_edge*> edge_vec;
	vector<vertex* > vertex_vec;
	vector<site> site_point;
	vector<face*> face_vec;
	read(number, site_point, test_point_number, test_point);
	sort(site_point.begin(), site_point.end(), sort_site);

	coast_node* start_node = new coast_node(site_point[0]);
	coast_line cl(start_node);
	long int site_event_now = 1;
	eventqueue circle_event_queue;

	face* face_to_add = new face(0);
	face_vec.push_back(face_to_add);
	bool init_same_y = true;
	while (site_event_now != number || circle_event_queue.getMax()) {
		bool site_event_sign;
		if (!circle_event_queue.getMax()) { site_event_sign = true; }
		else if (site_event_now == number) { site_event_sign = false; }
		else if (site_point[site_event_now].p < circle_event_queue.getMax()->eventpoint) { site_event_sign = false; }
		else site_event_sign = true;

		if (init_same_y && site_point[site_event_now].p.y == site_point[site_event_now - 1].p.y) {}
		else init_same_y = false;
		
		if (init_same_y) {
			cl.position = site_point[site_event_now].p.y;
			coast_node* to_replace = cl.insert_same_y(site_point[site_event_now], edge_vec, vertex_vec);
			face* face_to_add = new face(site_event_now);
			face_to_add->inc_edge = to_replace->edge;
			face_vec.push_back(face_to_add);
			if (site_event_now == 1) face_vec.at(0)->inc_edge = to_replace->edge->twin;
			delete to_replace;
			site_event_now++;
		}
		else {
			if (site_event_sign) {// site event
				cl.position = site_point[site_event_now].p.y;
				coast_node* to_replace = cl.insert(site_point[site_event_now], edge_vec);
			
				face* face_to_add = new face(site_event_now);
				face_to_add->inc_edge = to_replace->edge->twin;
				face_vec.push_back(face_to_add);
				if (site_event_now == 1) face_vec.at(0)->inc_edge = to_replace->edge;

				circle_event_queue.delete_element(to_replace->re_event); // delete original event;
				add_circle_event(to_replace->parent->pred, circle_event_queue, cl.position+1); // parent of replace is new inserted arc!
				add_circle_event(to_replace->parent->suc, circle_event_queue, cl.position+1); // circle event generated by site event should not be limited by coast line position
				delete to_replace;
				site_event_now++;
			}
			else {//circle event
				Event_circle* to_delete = circle_event_queue.delMax();
				cl.position = to_delete->eventpoint.y;
				coast_node* to_move = to_delete->node;
				coast_node* work = to_move->parent;
				double should_remove = work->break_point(to_delete->eventpoint.y) - to_delete->eventpoint.x;

				vertex* vt = new vertex(vertex_vec.size(), to_delete->center.x, to_delete->center.y);
				vt->r = to_delete->center.y - to_delete->eventpoint.y;
				vertex_vec.push_back(vt);

				coast_node* to_remove = cl.remove(to_delete, vt, edge_vec);
				circle_event_queue.delete_element(to_remove->pred->re_event);
				to_remove->pred->re_event = NULL;
				circle_event_queue.delete_element(to_remove->suc->re_event);
				to_remove->suc->re_event = NULL;

				add_circle_event(to_remove->pred, circle_event_queue, cl.position + 1e-5);
				add_circle_event(to_remove->suc, circle_event_queue, cl.position + 1e-5);

				delete to_delete;
			}
		}
	}

	cl.position -= 4000;
	coast_node* work = cl.root;
	vector<vertex* > out_vertex;
	traverse(work, cl.position, out_vertex);  // This is to set all out vertex

	for (auto f : face_vec) {
		half_edge* start = f->inc_edge;
		long int count = 0;
		while (start->pre && start->pre != f->inc_edge) {
			start = start->pre;
		}
		half_edge* record = start;
		do
		{
			start->inc_face = f;
			start = start->succ;
			count++;
		} while (start && start != record);
	}

	long int count = 1;
	for (auto q : test_point) { // This is to calculate the point in each hall; step by step method find the neighbor is closest to target. 
		face* test_face = face_vec.at(face_vec.size() / 2); //start from the center point
		bool stop = true;
		while (stop) {
			half_edge* start = test_face->inc_edge;
			while (start->pre && start->pre != test_face->inc_edge) {
				start = start->pre;
			}
			half_edge* record = start;
			if (!in_face(q, test_face) ) {
				double length_r = INFINITY;
				face* next = 0;
				int id_r;
				do
				{
					site m = site_point[start->twin->inc_face->id];
					double length = (m.p.x - q.x)*(m.p.x - q.x) + (m.p.y - q.y)*(m.p.y - q.y);
					if (length < length_r) {
						length_r = length;
						next = start->twin->inc_face;
						id_r = start->twin->inc_face->id;
					}
					start = start->succ;
				} while (start && start != record);
				test_face = next;
			  }
			  else {
						stop = false;
					}
		  }
    	  printf("%ld\r\n", site_point[test_face->id].id);
//		out << site_point[test_face->id].id << endl;
//		out << q.x << " " << q.y << endl;
//		out << site_point.at(test_face->id).p.x << " " << site_point.at(test_face->id).p.y << endl;
	}
	return 0;
}
