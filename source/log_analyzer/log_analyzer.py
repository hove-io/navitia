#! /usr/bin/python3
import plotly
import re, os, argparse


kraken_regex = re.compile(r".* - Api : (.+), worker : (.+), request : (.+), start : (\d+), end : (\d+)")

jormun_regex = re.compile(r".*Task : (.+), request : (.+),  start : (.+), end : (.+)")

generate_id_regex = re.compile(r".*Generating id : (.+) for request : (.+)")

request_regex = re.compile(r"journeys_(\w+)#(\w+)#(\w*)")


class KrakenTask:
    def __init__(self, api, sub_request_id, worker, start, end):
        self.api = api
        self.sub_request_id = sub_request_id
        self.worker = worker
        self.start = int(start)
        self.end = int(end)


class JormunTask:
    def __init__(self, task_name, sub_request_id, start, end):
        self.task_name = task_name
        self.sub_request_id = sub_request_id
        self.start = int(start)
        self.end = int(end)


class Request:
    def __init__(self, request_id, scenario):
        self.request_id = request_id
        self.scenario = scenario
        self.kraken_tasks_by_worker = {}  # dict worker -> list of tasks
        self.jormun_tasks = []
        self.url = ""

    def add_kraken_task(self, api, sub_request_id, worker, start, end):
        worker_tasks = self.kraken_tasks_by_worker.get(worker, [])
        worker_tasks.append(KrakenTask(api, sub_request_id, worker, start, end))
        self.kraken_tasks_by_worker[worker] = worker_tasks

    def add_jormun_task(self, task_name, sub_request_id, start, end):
        self.jormun_tasks.append(JormunTask(task_name, sub_request_id, start, end))

    def set_url(self, url):
        self.url = url

    def myprint(self):
        print("Request : " + self.request_id + " scenario : " + self.scenario + " url : " + self.url)
        for worker, tasks in self.kraken_tasks_by_worker.items():
            print("   Worker : " + worker)
            for task in tasks:
                print(
                    "      Api : " + task.api + " sub_request : " + task.sub_request_id + " start : ",
                    task.start,
                    " end : ",
                    task.end,
                )
        print("   Jormun : ")
        for task in self.jormun_tasks:
            print(
                "      Task : " + task.task_name + " sub_request : " + task.sub_request_id + " start : ",
                task.start,
                " end : ",
                task.end,
            )

    def create_gantt(self, output_dir):
        # https://plotly.com/python/gantt/
        import plotly.figure_factory

        sorted_jormun_tasks = sorted(self.jormun_tasks, key=lambda task: (task.start, -task.end))
        df = [
            dict(
                Task="J{}#{}".format(task.sub_request_id, task.task_name),
                Start=task.start,
                Finish=task.end,
                SubRequest="J{}#{}".format(task.sub_request_id, task.task_name),
            )
            for task in sorted_jormun_tasks
        ]
        for worker, tasks in self.kraken_tasks_by_worker.items():
            for task in tasks:
                df.append(
                    dict(
                        Task="Kraken_{}".format(task.worker),
                        Start=task.start,
                        Finish=task.end,
                        SubRequest="K{}".format(task.sub_request_id),
                    )
                )

        import colorsys

        nb_of_colors = len(df)
        HSV_tuples = [(x * 1.0 / nb_of_colors, 0.5, 0.5) for x in range(nb_of_colors)]
        RGB_tuples = [colorsys.hsv_to_rgb(*x) for x in HSV_tuples]

        fig = plotly.figure_factory.create_gantt(
            df, index_col='SubRequest', group_tasks=True, colors=RGB_tuples, show_colorbar=True, title=self.url
        )

        fig.update_layout(hoverlabel=dict(bgcolor="white", font_size=16, font_family="Rockwell", namelength=-1))
        html_str = plotly.io.to_html(fig)
        html_filename = "{}_{}.html".format(self.request_id, self.scenario)
        filepath = os.path.join(output_dir, html_filename)
        os.makedirs(output_dir, exist_ok=True)
        with open(filepath, 'w') as html_file:
            html_file.write(html_str)
        print("Created ", filepath)
        # fig.show(renderer="png")


def run(kraken_log_filepath, jormun_log_filepath, output_dir):
    requests = {}  # dict (request_id, scenario) -> Request
    with open(kraken_log_filepath) as kraken_log_file:
        for l in kraken_log_file.readlines():
            match = kraken_regex.match(l)
            if match:
                api = match.group(1)
                worker = match.group(2)
                kraken_request_id = match.group(3)
                start = match.group(4)
                end = match.group(5)
                # print("Api : " + api + " worker : " + worker + " request : " + kraken_request_id + " start : " + start + " end : " + end)
                request_match = request_regex.match(kraken_request_id)
                if request_match:
                    request_id = request_match.group(1)
                    scenario = request_match.group(2)
                    sub_request_id = request_match.group(3)
                    # print("request_id : " + request_id + " scenario : " + scenario + " sub_request : " + sub_request_id)
                    request = requests.get((request_id, scenario), Request(request_id, scenario))
                    request.add_kraken_task(api, sub_request_id, worker, start, end)
                    requests[(request_id, scenario)] = request

    with open(jormun_log_filepath) as jormun_log_file:
        for l in jormun_log_file.readlines():
            match = jormun_regex.match(l)
            if match:
                task_name = match.group(1)
                kraken_request_id = match.group(2)
                start = match.group(3)
                end = match.group(4)
                request_match = request_regex.match(kraken_request_id)
                if request_match:
                    request_id = request_match.group(1)
                    scenario = request_match.group(2)
                    sub_request_id = request_match.group(3)
                    request = requests.get((request_id, scenario), Request(request_id, scenario))
                    request.add_jormun_task(task_name, sub_request_id, start, end)
                    requests[(request_id, scenario)] = request
            match = generate_id_regex.match(l)
            if match:
                kraken_request_id = match.group(1)
                url = match.group(2)
                request_match = request_regex.match(kraken_request_id)
                if request_match:
                    request_id = request_match.group(1)
                    scenario = request_match.group(2)
                    request = requests.get((request_id, scenario), Request(request_id, scenario))
                    request.set_url(url)
                    requests[(request_id, scenario)] = request

    for request in requests.values():
        request.create_gantt(output_dir)
        # request.myprint()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--kraken_log_file", required=True, nargs=1)
    parser.add_argument("--jormun_log_file", required=True, nargs=1)
    parser.add_argument("--output_dir", required=True, nargs=1)
    args = parser.parse_args()
    run(args.kraken_log_file[0], args.jormun_log_file[0], args.output_dir[0])


if __name__ == '__main__':
    main()
